/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/22 18:19:10 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include "../utils/Utils.hpp"
#include <sstream>
#include <fcntl.h>
#include <climits>
#include <stdio.h>
#include <sys/select.h>
#include <algorithm>
#include <iostream>
#include <sys/poll.h>

Logger &CGIHandler::_logger = Logger::getInstance();

CGIHandler::CGIHandler() {
	char* cwd = getcwd(NULL, 0);
	if (cwd) {
		_cwd = cwd;
		free(cwd);
	}
	_logger.configure("logs/cgi.log", INFO, true, true);
}

CGIHandler::~CGIHandler() {
	_envMap.clear();
}

Response CGIHandler::executeCGI(const HTTPRequest& request,
								const std::string& cgiPath,
								const std::string& scriptPath) {
	_logger.info("=== Starting CGI Execution ===");
	_logger.info("CGI Path: " + cgiPath);
	_logger.info("Script Path: " + scriptPath);
	_logger.info("Request Body Size: " + Utils::numToString(request.getBody().length()));
	setupEnvironment(request, scriptPath);
	char** env = createEnvArray();
	if (!env) {
		return createErrorResponse(500, "Failed to setup environment");
	}

	// Create pipes with larger buffer
	int output_pipe[2];
	if (pipe(output_pipe) < 0) {
		cleanup(env);
		return createErrorResponse(500, "Failed to create pipe");
	}

#ifdef F_SETPIPE_SZ
	fcntl(output_pipe[1], F_SETPIPE_SZ, 1048576);  // Set to 1MB
#endif

	// Create temp file
	char tempPath[] = "/tmp/webserv_cgi_XXXXXX";
	int tempFd = mkstemp(tempPath);
	if (tempFd < 0) {
		cleanup(env);
		close(output_pipe[0]);
		close(output_pipe[1]);
		return createErrorResponse(500, "Failed to create temp file");
	}

	// Write body in smaller chunks
	const std::string& body = request.getBody();
	const size_t CHUNK_SIZE = 8192;  // Smaller chunks
	size_t totalWritten = 0;

	while (totalWritten < body.length()) {
		size_t toWrite = std::min(CHUNK_SIZE, body.length() - totalWritten);
		ssize_t written = write(tempFd, body.c_str() + totalWritten, toWrite);
		if (written < 0) {
			cleanup(env);
			close(tempFd);
			close(output_pipe[0]);
			close(output_pipe[1]);
			unlink(tempPath);
			return createErrorResponse(500, "Failed to write to temp file");
		}
		totalWritten += written;
	}

	// Clear request body and reset file position
	const_cast<HTTPRequest&>(request).clearBody();
	lseek(tempFd, 0, SEEK_SET);

	_logger.info("Executing CGI: " + cgiPath);
	pid_t pid = fork();
	if (pid < 0) {
		cleanup(env);
		close(tempFd);
		close(output_pipe[0]);
		close(output_pipe[1]);
		unlink(tempPath);
		return createErrorResponse(500, "Fork failed");
	}
	for (int i = 0; i < 2; ++i) {
		int flags = fcntl(output_pipe[i], F_GETFL, 0);
		fcntl(output_pipe[i], F_SETFL, flags | O_NONBLOCK);
	}
	if (pid == 0) {
		// Child process
		close(output_pipe[0]);

		if (dup2(tempFd, STDIN_FILENO) < 0) {
			_logger.error("Failed to redirect stdin");
			_exit(1);
		}

		if (dup2(output_pipe[1], STDOUT_FILENO) < 0) {
			_logger.error("Failed to redirect stdout");
			_exit(1);
		}

		close(tempFd);
		close(output_pipe[1]);

		execve(cgiPath.c_str(), (char*[]){
				const_cast<char*>(cgiPath.c_str()),
				const_cast<char*>(scriptPath.c_str()),
				NULL
		}, env);

		_logger.error("execve failed: " + std::string(strerror(errno)));
		_exit(1);
	}

	// Parent process
	cleanup(env);
	close(tempFd);
	close(output_pipe[1]);
	unlink(tempPath);

	Response response = handleCGIOutput(output_pipe[0], pid, 30);
	close(output_pipe[0]);

	return response;
}

Response CGIHandler::handleCGIOutput(int output_fd, pid_t pid, int timeout_seconds) {
	char tempPath[] = "/tmp/webserv_cgi_out_XXXXXX";
	int raw_fd = mkstemp(tempPath);
	if (raw_fd < 0) {
		_logger.error("Failed to create temp file for CGI output");
		kill(pid, SIGTERM);
		return createErrorResponse(500, "Failed to create temp file");
	}
	unlink(tempPath);

	bool process_done = false;
	time_t start_time = time(NULL);

	while (true) {
		if (!process_done) {
			int status;
			pid_t result = waitpid(pid, &status, WNOHANG);
			if (result == pid) {
				process_done = true;
				if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
					close(raw_fd);
					return createErrorResponse(500, "CGI process failed");
				}
			} else if (result < 0) {
				close(raw_fd);
				return createErrorResponse(500, "waitpid failed");
			}
		}

		struct pollfd pfd;
		pfd.fd = output_fd;
		pfd.events = POLLIN;
		int ready = poll(&pfd, 1, 100);  // 100ms timeout

		if (ready > 0) {
			char buffer[8192];
			ssize_t bytes = read(output_fd, buffer, sizeof(buffer));
			if (bytes > 0) {
				ssize_t written = write(raw_fd, buffer, bytes);
				if (written != bytes) {
					close(raw_fd);
					kill(pid, SIGTERM);
					return createErrorResponse(500, "Failed to write CGI output");
				}
			} else if (bytes == 0 && process_done) {
				break;
			}
		}

		if (time(NULL) - start_time > timeout_seconds) {
			close(raw_fd);
			kill(pid, SIGTERM);
			return createErrorResponse(504, "CGI Timeout");
		}
	}

	// Reset file position and read entire output
	lseek(raw_fd, 0, SEEK_SET);
	std::string output;
	char buffer[8192];
	ssize_t bytes;
	while ((bytes = read(raw_fd, buffer, sizeof(buffer))) > 0) {
		output.append(buffer, bytes);
	}
	close(raw_fd);

	return parseCGIOutput(output);
}

void CGIHandler::cleanup(char** env) {
	if (env) {
		for (int i = 0; env[i] != NULL; i++) {
			delete[] env[i];
		}
		delete[] env;
	}
}

void CGIHandler::setupEnvironment(const HTTPRequest &request, const std::string &scriptPath) {
	_envMap.clear();

	// Special header handling
	if (request.hasHeader("X-Secret-Header-For-Test")) {
		_envMap["HTTP_X_SECRET_HEADER_FOR_TEST"] = request.getHeader("X-Secret-Header-For-Test");
	}
	// Add content info for POST
	if (request.getMethod() == "POST") {
		_envMap["CONTENT_LENGTH"] = Utils::numToString(request.getBody().length());
		_envMap["CONTENT_TYPE"] = request.getHeader("Content-Type");
	}
	// Critical environment variables
	_envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	_envMap["REQUEST_METHOD"] = request.getMethod();
	_envMap["PATH_INFO"] = request.getPath();
	_envMap["PATH_TRANSLATED"] = scriptPath;
	_envMap["QUERY_STRING"] = "";
	_envMap["REMOTE_ADDR"] = "127.0.0.1";
	_envMap["REQUEST_URI"] = request.getPath();
	_envMap["SCRIPT_FILENAME"] = scriptPath;
	_envMap["SCRIPT_NAME"] = request.getPath();
	_envMap["SERVER_NAME"] = "localhost";
	_envMap["SERVER_PORT"] = "8080";
	_envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	_envMap["SERVER_SOFTWARE"] = "webserv/1.0";
	_envMap["REDIRECT_STATUS"] = "200";
}

char** CGIHandler::createEnvArray() {
    try {
        size_t envSize = _envMap.size();
        char** env = new char*[envSize + 1];
        size_t i = 0;

        for (std::map<std::string, std::string>::const_iterator it = _envMap.begin();
             it != _envMap.end(); ++it, ++i) {
            std::string envStr = it->first + "=" + it->second;
            env[i] = new char[envStr.length() + 1];
            std::strcpy(env[i], envStr.c_str());
        }
        env[envSize] = NULL;
        return env;
    } catch (const std::exception& e) {
        _logger.error("Failed to create environment array: " + std::string(e.what()));
        return NULL;
    }
}

Response CGIHandler::createErrorResponse(int code, const std::string& message) {
	Response response(code);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>" + message + "</h1></body></html>");
	return response;
}

void CGIHandler::setNonBlocking(int fd) const {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		throw std::runtime_error("Failed to get file descriptor flags");
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		throw std::runtime_error("Failed to set non-blocking mode");
	}
}

Response CGIHandler::parseCGIOutput(const std::string& output) {
	// Find headers end
	size_t headerEnd = output.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		headerEnd = output.find("\n\n");
		if (headerEnd == std::string::npos) {
			// No headers found, treat entire output as body
			Response response(200);
			response.addHeader("Content-Type", "text/plain");
			response.setBody(output);
			return response;
		}
	}

	// Parse headers
	std::string headers = output.substr(0, headerEnd);
	// Skip the separator and any leading whitespace in body
	size_t bodyStart = headerEnd + (output[headerEnd+1] == '\n' ? 2 : 4);
	while (bodyStart < output.length() &&
		   (output[bodyStart] == '\r' || output[bodyStart] == '\n')) {
		bodyStart++;
	}
	std::string body = output.substr(bodyStart);

	Response response(200); // Default status code
	std::istringstream headerStream(headers);
	std::string line;

	bool statusSet = false;
	while (std::getline(headerStream, line)) {
		// Remove \r if present
		if (!line.empty() && line[line.length()-1] == '\r') {
			line = line.substr(0, line.length()-1);
		}

		if (line.empty()) continue;

		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos) {
			// Check for Status line
			if (line.find("Status:") == 0) {
				std::string statusStr = line.substr(7);
				size_t spacePos = statusStr.find(' ');
				if (spacePos != std::string::npos) {
					int status = std::atoi(statusStr.substr(0, spacePos).c_str());
					if (status >= 100 && status < 600) {
						response.setStatusCode(status);
						statusSet = true;
					}
				}
			}
			continue;
		}

		std::string name = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 1);

		// Trim leading/trailing whitespace from value
		while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
			value = value.substr(1);
		}

		response.addHeader(name, value);
	}

	// Set default status and content type if not set
	if (!statusSet && !response.hasHeader("Status")) {
		response.setStatusCode(200);
	}
	if (!response.hasHeader("Content-Type")) {
		response.addHeader("Content-Type", "text/plain");
	}

	response.setBody(body);
	return response;
}
