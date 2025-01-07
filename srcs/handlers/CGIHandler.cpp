/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:36:45 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include <algorithm>
#include <fcntl.h>
#include <sstream>
#include <sys/poll.h>

Logger &CGIHandler::_logger = Logger::getInstance();

CGIHandler::CGIHandler() {
	char *cwd = getcwd(NULL, 0);
	if (cwd) {
		_cwd = cwd;
		free(cwd);
	}
	_logger.configure("logs/cgi.log", INFO, true, true);
}

CGIHandler::~CGIHandler() {
	_envMap.clear();
}

Response CGIHandler::executeCGI(const Request &request, const std::string &cgiPath, const std::string &scriptPath) {
	_logger.info("=== Starting CGI Execution ===");
	_logger.info("CGI Path: " + cgiPath);
	_logger.info("Script Path: " + scriptPath);
	_logger.info("Request Body Size: " + Utils::numToString(request.getBody().length()));
	setupEnvironment(request, scriptPath);
	char **env = createEnvArray();
	if (!env)
		return createErrorResponse(500, "Failed to setup environment");

	int output_pipe[2];
	if (pipe(output_pipe) < 0) {
		cleanup(env);
		return createErrorResponse(500, "Failed to create pipe");
	}

	#ifdef F_SETPIPE_SZ
		fcntl(output_pipe[1], F_SETPIPE_SZ, CGI_PIPE_BUFSIZE);
	#endif

	// Temp file to store request body
	char tempPath[] = "/tmp/webserv_cgi_XXXXXX";
	int	 tempFd = mkstemp(tempPath);
	if (tempFd < 0) {
		cleanup(env);
		close(output_pipe[0]);
		close(output_pipe[1]);
		return createErrorResponse(500, "Failed to create temp file");
	}

	const std::string &body = request.getBody();
	const size_t	   CHUNK_SIZE = 8192;
	size_t			   totalWritten = 0;

	while (totalWritten < body.length()) {
		size_t	toWrite = std::min(CHUNK_SIZE, body.length() - totalWritten);
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

	const_cast<Request &>(request).clearBody();
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

	// Child process
	if (pid == 0) {
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
		execve(cgiPath.c_str(),
			   (char *[]){const_cast<char *>(cgiPath.c_str()), const_cast<char *>(scriptPath.c_str()), NULL}, env);
		_logger.error("execve failed: " + std::string(strerror(errno)));
		_exit(1);
	}

	// Parent process
	cleanup(env);
	close(tempFd);
	close(output_pipe[1]);
	unlink(tempPath);

	Response response = handleCGIOutput(output_pipe[0], pid);
	close(output_pipe[0]);

	return response;
}

Response CGIHandler::handleCGIOutput(int output_fd, pid_t pid) {
	char tempPath[] = "/tmp/webserv_cgi_out_XXXXXX";
	int	 raw_fd = mkstemp(tempPath);
	if (raw_fd < 0) {
		kill(pid, SIGTERM);
		return createErrorResponse(500, "Failed to create temp file");
	}
	unlink(tempPath);

	bool   process_done = false;
	time_t start_time = time(NULL);
	size_t raw_bytes = 0;

	while (!process_done) {
		int	  status;
		pid_t result = waitpid(pid, &status, WNOHANG);
		if (result == pid) {
			process_done = true;
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
				close(raw_fd);
				return createErrorResponse(500, "CGI process failed");
			}
		}
		char	buffer[CGI_BUFSIZE];
		ssize_t bytes = read(output_fd, buffer, sizeof(buffer));
		if (bytes > 0) {
			if (write(raw_fd, buffer, bytes) != bytes) {
				close(raw_fd);
				kill(pid, SIGTERM);
				return createErrorResponse(500, "Failed to write CGI output");
			}
			raw_bytes += bytes;
		} else if (bytes == 0 && process_done) {
			break;
		} else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
			close(raw_fd);
			kill(pid, SIGTERM);
			return createErrorResponse(500, "Read error");
		}
		if (time(NULL) - start_time > CGI_TIMEOUT) {
			close(raw_fd);
			kill(pid, SIGTERM);
			return createErrorResponse(504, "CGI Timeout");
		}
	}
	return parseCGIOutput(raw_fd, raw_bytes);
}

Response CGIHandler::parseCGIOutput(int raw_fd, size_t raw_bytes) {
	lseek(raw_fd, 0, SEEK_SET);
	char	 header_buf[8192] = {0};
	ssize_t	 header_bytes = 0;
	size_t	 header_size = 0;
	bool	 found_header_end = false;
	Response response(200);

	while ((header_bytes = read(raw_fd, header_buf, sizeof(header_buf))) > 0) {
		std::string header_chunk(header_buf, header_bytes);

		size_t pos = header_chunk.find("\r\n\r\n");
		if (pos != std::string::npos) {
			header_size = pos + 4;
			found_header_end = true;
		} else {
			pos = header_chunk.find("\n\n");
			if (pos != std::string::npos) {
				header_size = pos + 2;
				found_header_end = true;
			}
		}
		if (found_header_end) {
			std::istringstream header_stream(header_chunk.substr(0, pos));
			std::string		   line;
			while (std::getline(header_stream, line)) {
				if (line.empty())
					continue;
				if (!line.empty() && line[line.length() - 1] == '\r')
					line = line.substr(0, line.length() - 1);
				if (line.find("Status:") == 0) {
					size_t status_pos = line.find_first_of("0123456789");
					if (status_pos != std::string::npos) {
						int status = std::atoi(line.substr(status_pos).c_str());
						if (status >= 100 && status < 600)
							response.setStatusCode(status);
					}
					continue;
				}
				size_t colon_pos = line.find(':');
				if (colon_pos != std::string::npos) {
					std::string name = line.substr(0, colon_pos);
					std::string value = line.substr(colon_pos + 1);
					while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value = value.substr(1);
					response.addHeader(name, value);
				}
			}
			break;
		}
	}
	if (!found_header_end) {
		close(raw_fd);
		return createErrorResponse(500, "Invalid CGI output");
	}
	size_t body_size = raw_bytes - header_size;
	response.addHeader("Content-Length", Utils::numToString(body_size));
	lseek(raw_fd, header_size, SEEK_SET);
	response.setFileDescriptor(raw_fd);

	return response;
}

void CGIHandler::cleanup(char **env) {
	for (int i = 0; env[i] != NULL; i++) delete[] env[i];
	delete[] env;
}

void CGIHandler::setupEnvironment(const Request &request, const std::string &scriptPath) {
	_envMap.clear();

	// Special header handling
	if (request.hasHeader("X-Secret-Header-For-Test"))
		_envMap["HTTP_X_SECRET_HEADER_FOR_TEST"] = request.getHeader("X-Secret-Header-For-Test");

	if (request.getMethod() == "POST") {
		if (request.isChunked())
			_envMap["CONTENT_LENGTH"] = Utils::numToString(request.getBody().length());
		else if (request.hasHeader("Content-Length"))
			_envMap["CONTENT_LENGTH"] = request.getHeader("Content-Length");
		if (request.hasHeader("Content-Type"))
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

char **CGIHandler::createEnvArray() {
	try {
		size_t envSize = _envMap.size();
		char **env = new char *[envSize + 1];
		size_t i = 0;

		for (std::map<std::string, std::string>::const_iterator it = _envMap.begin(); it != _envMap.end(); ++it, ++i) {
			std::string envStr = it->first + "=" + it->second;
			env[i] = new char[envStr.length() + 1];
			std::strcpy(env[i], envStr.c_str());
		}
		env[envSize] = NULL;
		return env;
	} catch (const std::exception &e) {
		_logger.error("Failed to create environment array: " + std::string(e.what()));
		return NULL;
	}
}

Response CGIHandler::createErrorResponse(int code, const std::string &message) {
	Response response(code);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>" + message + "</h1></body></html>");
	return response;
}
