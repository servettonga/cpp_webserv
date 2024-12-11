/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/12 00:05:00 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include "../utils/Utils.hpp"
#include "../server/ServerConfig.hpp"
#include <sstream>
#include <iostream>

CGIHandler::CGIHandler() {
	_envMap.clear();

	char *cwd = std::getenv("PWD");
	if (cwd)
		_workingDir = cwd;
	else
		_workingDir = ".";
}

CGIHandler::~CGIHandler() {}

Response CGIHandler::executeCGI(const HTTPRequest &request, const std::string &scriptPath) {
	const ServerConfig* config = static_cast<const ServerConfig*>(request.getConfig());
	if (!config) {
		std::cerr << "No server config available" << std::endl;
		return Response::makeErrorResponse(500);
	}

	setupEnvironment(request, scriptPath);
	int inputPipe[2], outputPipe[2];

	if (!createPipes(inputPipe, outputPipe))
		return Response::makeErrorResponse(500);

	char **env = createEnvArray();
	pid_t pid = fork();

	if (pid < 0) {
		cleanupPipes(inputPipe, outputPipe);
		freeEnvArray(env);
		return Response::makeErrorResponse(500);
	}
	if (pid == 0)
		return handleChildProcess(request, scriptPath, inputPipe, outputPipe, env);

	return handleParentProcess(pid, request, inputPipe, outputPipe, env);
}

bool CGIHandler::createPipes(int inputPipe[2], int outputPipe[2]) {
	if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0) {
		std::cerr << "Error creating pipes: " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}

void CGIHandler::cleanupPipes(int inputPipe[2], int outputPipe[2]) {
	close(inputPipe[0]);
	close(inputPipe[1]);
	close(outputPipe[0]);
	close(outputPipe[1]);
}

Response CGIHandler::handleChildProcess(const HTTPRequest &request, const std::string &scriptPath,
										int inputPipe[2], int outputPipe[2], char **env) {
	close(inputPipe[1]);
	close(outputPipe[0]);

	if (dup2(inputPipe[0], STDIN_FILENO) < 0 ||
		dup2(outputPipe[1], STDOUT_FILENO) < 0) {
		std::cerr << "Error setting up file descriptors: " << strerror(errno) << std::endl;
		exit(1);
	}

	close(inputPipe[0]);
	close(outputPipe[1]);

	if (scriptPath.find(".py") != std::string::npos) {
		execle("/usr/bin/python3", "python3", scriptPath.c_str(), NULL, env);
	} else {
		const ServerConfig* config = static_cast<const ServerConfig*>(request.getConfig());
		const LocationConfig* location = config->getLocation(request.getPath());
		if (location && !location->cgi_path.empty()) {
			execle(location->cgi_path.c_str(), location->cgi_path.c_str(), scriptPath.c_str(), NULL, env);
		}
	}

	std::cerr << "Execle failed: " << strerror(errno) << std::endl;
	exit(1);
}

Response CGIHandler::handleParentProcess(pid_t pid, const HTTPRequest &request,
										 int inputPipe[2], int outputPipe[2], char **env) {
	close(inputPipe[0]);
	close(outputPipe[1]);

	writeToPipe(inputPipe[1], request.getBody());
	close(inputPipe[1]);

	std::string output = readFromPipe(outputPipe[0]);
	close(outputPipe[0]);

	int status;
	waitpid(pid, &status, 0);
	freeEnvArray(env);

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		Response response(200);
		parseOutput(output, response);
		return response;
	}

	return Response::makeErrorResponse(500);
}

void CGIHandler::setupEnvironment(const HTTPRequest &request, const std::string &scriptPath) {
	_envMap.clear();

	// Standard CGI variables
	_envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	_envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	_envMap["REQUEST_METHOD"] = request.getMethod();
	_envMap["SCRIPT_FILENAME"] = scriptPath;
	_envMap["PATH_INFO"] = request.getPath();
	_envMap["PATH"] = "/usr/local/bin:/usr/bin:/bin";

	// Query string
	std::string path = request.getPath();
	size_t pos = path.find("?");
	_envMap["QUERY_STRING"] = (pos != std::string::npos) ? path.substr(pos + 1) : "";

	// Content info
	_envMap["CONTENT_LENGTH"] = Utils::numToString(request.getBody().length());
	_envMap["CONTENT_TYPE"] = request.getHeader("Content-Type");

	// HTTP headers
	for (std::map<std::string, std::string>::const_iterator it = request.getHeaders().begin();
		 it != request.getHeaders().end(); ++it) {
		std::string headerName = it->first;
		_envMap["HTTP_" + headerName] = it->second;
	}
}

char **CGIHandler::createEnvArray() {
	size_t envSize = _envMap.size() + 1;

	char **envArray = new char*[envSize];
	
	size_t i = 0;
	for (std::map<std::string, std::string>::const_iterator it = _envMap.begin(); it != _envMap.end(); ++it) {
		std::string envVar = it->first + "=" + it->second;
		envArray[i] = new char[envVar.length() + 1];
		std::strcpy(envArray[i], envVar.c_str());
		++i;
	}
	envArray[i] = NULL;

	return envArray;
}

void CGIHandler::freeEnvArray(char **env) {
	for (int i = 0; env[i] != NULL; ++i)
		delete[] env[i];
	delete[] env;
}

std::string CGIHandler::readFromPipe(int fd) {
	char buffer[4096];
	std::string result;
	ssize_t bytesRead;

	while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
		result.append(buffer, bytesRead);

	if (bytesRead < 0)
		std::cerr << "Error reading from pipe: " << strerror(errno) << std::endl;

	return result;
}

void CGIHandler::writeToPipe(int fd, const std::string &data) {
	size_t dataSize = data.size();
	size_t written = 0;
	
	while (written < dataSize) {
		ssize_t bytesWritten = write(fd, data.c_str() + written, dataSize - written);
		
		if (bytesWritten < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				std::cerr << "Error writing to pipe: " << strerror(errno) << std::endl;
				break;
			}
		}
		written += bytesWritten;
	}
}

void CGIHandler::parseOutput(const std::string &output, Response &response) {
	// Find the end of headers (double newline)
	size_t headerEnd = output.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		headerEnd = output.find("\n\n");
		if (headerEnd == std::string::npos) {
			// No headers found, treat the entire output as body
			response.setBody(output);
			return;
		}
	}

	// Parse headers
	std::string headers = output.substr(0, headerEnd);
	std::istringstream headerStream(headers);
	std::string line;

	while (std::getline(headerStream, line)) {
		if (!line.empty() && line[line.length()-1] == '\r')
			line = line.substr(0, line.length()-1);
		if (line.empty()) continue;

		// Split header into name and value
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) {
			std::string name = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);

			while (!value.empty() && isspace(value[0]))
				value = value.substr(1);

			response.addHeader(name, value);
		}
	}

	// Set body (skip headers and the separating newlines)
	size_t bodyStart = headerEnd + (output.substr(headerEnd).find("\n") + 1);
	std::string body = output.substr(bodyStart);
	response.setBody(body);
}
