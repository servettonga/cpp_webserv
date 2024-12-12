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

	// Get file extension and handler
	size_t extPos = scriptPath.find_last_of('.');
	std::string ext = scriptPath.substr(extPos);

	const ServerConfig* config = static_cast<const ServerConfig*>(request.getConfig());
	if (!config) {
		std::cerr << "No server config" << std::endl;
		exit(1);
	}

	std::map<std::string, std::string>::const_iterator handlerIt = config->cgi_handlers.find(ext);
	if (handlerIt != config->cgi_handlers.end()) {
		std::string handler = handlerIt->second;
		std::cout << "Executing CGI handler: " << handler << " for " << scriptPath << std::endl;

		if (ext == ".php") {
			char **argv = new char*[3];
			argv[0] = strdup("/usr/bin/php-cgi");  // Full path to php-cgi
			argv[1] = strdup(scriptPath.c_str());
			argv[2] = NULL;

			// Add PHP-specific environment variables
			setenv("REDIRECT_STATUS", "200", 1);
			setenv("SCRIPT_FILENAME", scriptPath.c_str(), 1);
			setenv("REQUEST_METHOD", request.getMethod().c_str(), 1);
			setenv("QUERY_STRING", "", 1);

			execve(argv[0], argv, env);

			// Clean up if execve fails
			std::cerr << "PHP-CGI execve failed: " << strerror(errno) << std::endl;
			free(argv[0]);
			free(argv[1]);
			delete[] argv;
		} else {
			std::vector<std::string> args;
			std::istringstream iss(handler);
			std::string part;
			while (iss >> part)
				args.push_back(part);
			args.push_back(scriptPath);

			char **argv = new char*[args.size() + 1];
			for (size_t i = 0; i < args.size(); ++i)
				argv[i] = strdup(args[i].c_str());
			argv[args.size()] = NULL;

			execve(argv[0], argv, env);

			for (size_t i = 0; argv[i]; ++i)
				free(argv[i]);
			delete[] argv;
		}
	}

	std::cerr << "No handler found for extension: " << ext << std::endl;
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
	_envMap["REDIRECT_STATUS"] = "200";  // Required for PHP
	_envMap["PATH_INFO"] = request.getPath();
	_envMap["PATH"] = "/usr/local/bin:/usr/bin:/bin";

	// PHP-specific variables
	_envMap["PHP_SELF"] = request.getPath();
	_envMap["SCRIPT_NAME"] = request.getPath();
	_envMap["REQUEST_URI"] = request.getPath();
	_envMap["DOCUMENT_ROOT"] = "www";

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
