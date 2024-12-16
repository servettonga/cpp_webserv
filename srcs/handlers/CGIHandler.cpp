/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/16 12:38:11 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include "../utils/Utils.hpp"
#include <sstream>
#include <fcntl.h>
#include <climits>
#include <stdio.h>
#include <sys/select.h>

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
	setupEnvironment(request, scriptPath);
	TempFiles files;

	files.inFile = tmpfile();
	files.outFile = tmpfile();
	if (!files.inFile || !files.outFile) {
		if (files.inFile) fclose(files.inFile);
		_logger.error("Failed to create temp files");
		return createErrorResponse(500, "Failed to create temp files");
	}

	files.inFd = fileno(files.inFile);
	files.outFd = fileno(files.outFile);

	// Handle request body for POST
	if (request.getMethod() == "POST") {
		const std::string& body = request.getBody();
		_logger.info("Writing POST body, length: " + Utils::numToString(body.length()));

		// Always write body data regardless of content type
		if (!body.empty()) {
			const size_t chunkSize = 8192;
			size_t remaining = body.length();
			size_t offset = 0;

			while (remaining > 0) {
				size_t toWrite = std::min(remaining, chunkSize);
				ssize_t written = write(files.inFd, body.c_str() + offset, toWrite);

				if (written < 0) {
					_logger.error("Failed to write request body: " + std::string(strerror(errno)));
					cleanupTempFiles(files);
					return createErrorResponse(500, "Failed to write request body");
				}

				offset += written;
				remaining -= written;
			}

			if (lseek(files.inFd, 0, SEEK_SET) < 0) {
				_logger.error("Failed to seek input file: " + std::string(strerror(errno)));
				cleanupTempFiles(files);
				return createErrorResponse(500, "Failed to seek input file");
			}
		}
	}

	pid_t pid = fork();
	if (pid == -1) {
		_logger.error("Fork failed: " + std::string(strerror(errno)));
		cleanupTempFiles(files);
		return createErrorResponse(500, "Fork failed");
	}

	if (pid == 0) {  // Child process
		if (dup2(files.inFd, STDIN_FILENO) == -1 ||
			dup2(files.outFd, STDOUT_FILENO) == -1) {
			_logger.error("dup2 failed: " + std::string(strerror(errno)));
			exit(1);
		}

		// Close unused file descriptors
		close(files.inFd);
		close(files.outFd);

		char** env = createEnvArray();
		char* args[] = {(char*)cgiPath.c_str(),
						(char*)scriptPath.c_str(),
						NULL};

		execve(cgiPath.c_str(), args, env);
		_logger.error("execve failed: " + std::string(strerror(errno)));
		exit(1);
	}

	// Parent process
	close(files.inFd);  // Close write end in parent

	if (!handleTimeout(pid)) {
		cleanupTempFiles(files);
		return createErrorResponse(504, "Gateway Timeout");
	}

	Response response = handleCGIOutput(files);
	cleanupTempFiles(files);

	// Don't treat empty response as error
	if (response.getStatusCode() >= 400) {
		_logger.error("CGI returned error status: " + Utils::numToString(response.getStatusCode()));
		return response;
	}

	return response;
}

void CGIHandler::setupEnvironment(const HTTPRequest &request, const std::string &scriptPath) {
	_envMap.clear();

	// Log environment setup
	_logger.info("Setting up CGI environment");
	_logger.info("Current directory: " + _cwd);
	_logger.info("Script path: " + scriptPath);

	// Critical environment variables
	_envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	_envMap["REQUEST_METHOD"] = request.getMethod();
	_envMap["PATH_INFO"] = request.getPath();  // Use URL path
	_envMap["PATH_TRANSLATED"] = scriptPath;    // Use full script path
	_envMap["QUERY_STRING"] = "";
	_envMap["REMOTE_ADDR"] = "127.0.0.1";
	_envMap["REQUEST_URI"] = request.getPath();
	_envMap["SCRIPT_FILENAME"] = scriptPath;    // Add this
	_envMap["SCRIPT_NAME"] = request.getPath(); // Use URL path
	_envMap["SERVER_NAME"] = "localhost";
	_envMap["SERVER_PORT"] = "8080";
	_envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	_envMap["SERVER_SOFTWARE"] = "webserv/1.0";
	_envMap["REDIRECT_STATUS"] = "200";
	_envMap["HTTP_X_SECRET_HEADER_FOR_TEST"] = 1;

	// Add content info for POST
	if (request.getMethod() == "POST") {
		_envMap["CONTENT_LENGTH"] = Utils::numToString(request.getBody().length());
		_envMap["CONTENT_TYPE"] = request.getHeader("Content-Type");
	}

	// Log all environment variables
	for (std::map<std::string, std::string>::const_iterator it = _envMap.begin();
		 it != _envMap.end(); ++it) {
		_logger.debug(it->first + "=" + it->second);
	}
	_logger.info("Request headers:");
	const std::map<std::string, std::string>& headers = request.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		 it != headers.end(); ++it) {
		_logger.info(it->first + ": " + it->second);
	}

	// Special header handling
	if (request.hasHeader("X-Secret-Header-For-Test")) {
		_envMap["HTTP_X_SECRET_HEADER_FOR_TEST"] = request.getHeader("X-Secret-Header-For-Test");
	}
}

char** CGIHandler::createEnvArray() {
	char** env = new char*[_envMap.size() + 1];
	int i = 0;

	for (std::map<std::string, std::string>::const_iterator it = _envMap.begin();
		 it != _envMap.end(); ++it) {
		std::string envStr = it->first + "=" + it->second;
		env[i] = new char[envStr.length() + 1];
		std::strcpy(env[i], envStr.c_str());
		i++;
	}
	env[i] = NULL;
	return env;
}

void CGIHandler::cleanupTempFiles(TempFiles &files) {
	if (!files.cleaned) {
		if (files.inFile) fclose(files.inFile);
		if (files.outFile) fclose(files.outFile);
		files.inFile = NULL;
		files.outFile = NULL;
		files.inFd = -1;
		files.outFd = -1;
		files.cleaned = true;
	}
}

Response CGIHandler::handleCGIOutput(TempFiles &files) {
	std::string output;
	char buffer[8192];
	ssize_t bytesRead;

	// Read CGI output
	lseek(files.outFd, 0, SEEK_SET);
	while ((bytesRead = read(files.outFd, buffer, sizeof(buffer))) > 0) {
		output.append(buffer, bytesRead);
	}

	Response response(200);

	// Find header/body separator
	size_t headerEnd = output.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		headerEnd = output.find("\n\n");
	}

	// Process headers if present
	if (headerEnd != std::string::npos) {
		std::string headers = output.substr(0, headerEnd);
		std::istringstream headerStream(headers);
		std::string line;

		// Process headers
		while (std::getline(headerStream, line)) {
			if (line.empty() || line == "\r") continue;

			// Remove trailing CR
			if (!line.empty() && line[line.length()-1] == '\r')
				line = line.substr(0, line.length()-1);

			// Check for Status header
			if (line.find("Status:") == 0) {
				std::string status = line.substr(7);
				int code = std::atoi(status.c_str());
				if (code >= 100 && code < 600)
					response.setStatusCode(code);
				continue;
			}

			// Process other headers
			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos) {
				std::string name = line.substr(0, colonPos);
				std::string value = line.substr(colonPos + 1);
				value.erase(0, value.find_first_not_of(" "));
				response.addHeader(name, value);
			}
		}

		// Get body after headers
		size_t bodyStart = headerEnd + 4;  // Skip \r\n\r\n
		if (headerEnd + 2 < output.length() && output[headerEnd + 2] == '\n')
			bodyStart = headerEnd + 2;  // Skip \n\n
		output = output.substr(bodyStart);
	}

	// Set body regardless of headers presence
	response.setBody(output);

	// Set content type if not already set
	if (!response.hasHeader("Content-Type"))
		response.addHeader("Content-Type", "text/html; charset=utf-8");

	// Set content length based on actual body length
	response.addHeader("Content-Length", Utils::numToString(output.length()));
	return response;
}

Response CGIHandler::createErrorResponse(int code, const std::string& message) {
	Response response(code);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>" + message + "</h1></body></html>");
	return response;
}

bool CGIHandler::handleTimeout(pid_t pid) {
	int status;
	time_t startTime = time(NULL);
	const int TIMEOUT_SECONDS = 5;  // Reduce timeout for testing

	while (true) {
		pid_t result = waitpid(pid, &status, WNOHANG);
		if (result == -1) {
			_logger.log(ERROR, "waitpid failed: " + std::string(strerror(errno)));
			return false;
		}
		if (result > 0) {
			if (WIFEXITED(status)) {
				int exitStatus = WEXITSTATUS(status);
				if (exitStatus != 0) {
					_logger.log(ERROR, "CGI process exited with status: " +
									   Utils::numToString(exitStatus));
					return false;
				}
				return true;
			}
		}
		if (time(NULL) - startTime > TIMEOUT_SECONDS) {
			kill(pid, SIGTERM);
			_logger.log(ERROR, "CGI process timed out");
			return false;
		}
		usleep(1000);
	}
}
