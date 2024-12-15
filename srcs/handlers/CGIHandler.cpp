/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/15 13:09:51 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include "../utils/Utils.hpp"
#include "../server/ServerConfig.hpp"
#include "../utils/TempFileManager.hpp"
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <algorithm>
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
	_tmpPath = TempFileManager::getTempDir();
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

	// Create temp files directly
	files.inFile = tmpfile();
	files.outFile = tmpfile();

	if (!files.inFile || !files.outFile) {
		if (files.inFile) fclose(files.inFile);
		return createErrorResponse(500, "Failed to create temp files");
	}

	files.inFd = fileno(files.inFile);
	files.outFd = fileno(files.outFile);

	// Write request body directly to input file
	if (request.getMethod() == "POST") {
		const std::string& body = request.getBody();
		write(files.inFd, body.c_str(), body.length());
		lseek(files.inFd, 0, SEEK_SET);
	}

	pid_t pid = fork();
	if (pid == -1) {
		cleanupTempFiles(files);
		return createErrorResponse(500, "Fork failed");
	}

	if (pid == 0) {
		if (dup2(files.inFd, STDIN_FILENO) == -1 ||
			dup2(files.outFd, STDOUT_FILENO) == -1) {
			_logger.error("dup2 failed: " + std::string(strerror(errno)));
			exit(1);
		}

		char** env = createEnvArray();
		char* args[] = {(char*)cgiPath.c_str(),
						(char*)scriptPath.c_str(),
						NULL};

		execve(cgiPath.c_str(), args, env);
		_logger.error("execve failed: " + std::string(strerror(errno)));
		exit(1);
	}

	// Parent process
	if (!handleTimeout(pid)) {
		cleanupTempFiles(files);
		return createErrorResponse(504, "Gateway Timeout");
	}

	return handleCGIOutput(files);
}

void CGIHandler::setupEnvironment(const HTTPRequest &request, const std::string &scriptPath) {
	_envMap.clear();

	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		_logger.error("getcwd failed: " + std::string(strerror(errno)));
		return;
	}
	std::string currentDir(cwd);

	// Always set these regardless of method
	_envMap["CONTENT_LENGTH"] = "0";
	_envMap["CONTENT_TYPE"] = "text/plain";

	// Override for POST
	if (request.getMethod() == "POST") {
		_envMap["CONTENT_LENGTH"] = Utils::numToString(request.getBody().length());
		_envMap["CONTENT_TYPE"] = request.getHeader("Content-Type");
	}

	_envMap["REQUEST_METHOD"] = request.getMethod();
	_envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	_envMap["SCRIPT_NAME"] = request.getPath();
	_envMap["SCRIPT_FILENAME"] = currentDir + "/" + scriptPath;
	_envMap["PATH_INFO"] = request.getPath();
	_envMap["PATH_TRANSLATED"] = currentDir + "/" + scriptPath;
	_envMap["QUERY_STRING"] = "";
	_envMap["REQUEST_URI"] = request.getPath();
	_envMap["SERVER_SOFTWARE"] = "webserv";
	_envMap["SERVER_NAME"] = "localhost";
	_envMap["SERVER_PORT"] = "8080";
	_envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	_envMap["REMOTE_ADDR"] = "127.0.0.1";
	_envMap["REDIRECT_STATUS"] = "200";

	char* systemPath = getenv("PATH");
	_envMap["PATH"] = systemPath ? systemPath : "/usr/local/bin:/usr/bin:/bin";
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

void CGIHandler::cleanupTempFiles(const TempFiles& files) {
	// Close and automatically delete temp files
	if (files.inFile) fclose(files.inFile);
	if (files.outFile) fclose(files.outFile);
}

Response CGIHandler::handleCGIOutput(const TempFiles& files) {
	std::string output;
	char buffer[8192];
	ssize_t bytesRead;

	// Read CGI output
	lseek(files.outFd, 0, SEEK_SET);
	while ((bytesRead = read(files.outFd, buffer, sizeof(buffer))) > 0) {
		output.append(buffer, bytesRead);
	}

	Response response(200);
	std::string body;

	// Find header/body separator
	size_t headerEnd = output.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		headerEnd = output.find("\n\n");
	}

	if (headerEnd != std::string::npos) {
		// Process headers
		std::string headers = output.substr(0, headerEnd);
		std::istringstream headerStream(headers);
		std::string line;

		// Process each header line
		while (std::getline(headerStream, line)) {
			if (line.empty() || line == "\r") continue;

			if (line[line.length()-1] == '\r')
				line.erase(line.length()-1);

			if (line.find("Status:") == 0) {
				std::string status = line.substr(7);
				int code = std::atoi(status.c_str());
				if (code >= 100 && code < 600)
					response.setStatusCode(code);
				continue;
			}

			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos) {
				std::string name = line.substr(0, colonPos);
				std::string value = line.substr(colonPos + 1);
				value.erase(0, value.find_first_not_of(" "));
				response.addHeader(name, value);
			}
		}

		// Get body without any leading \r\n
		body = output.substr(headerEnd + (output[headerEnd + 1] == '\n' ? 2 : 4));
		while (!body.empty() && (body[0] == '\r' || body[0] == '\n')) {
			body.erase(0, 1);
		}
	} else {
		body = output;
	}

	response.setBody(body);
	response.addHeader("Content-Type", "text/html; charset=utf-8");
	response.addHeader("Content-Length", Utils::numToString(body.length()));

	cleanupTempFiles(files);
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
