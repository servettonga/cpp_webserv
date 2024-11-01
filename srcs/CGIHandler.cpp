/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/02 22:33:23 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <fcntl.h>
#include <cerrno>

CGIHandler::CGIHandler() {}

CGIHandler::~CGIHandler() {}

Response CGIHandler::executeCGI(const HTTPRequest& request,
								const std::string& scriptPath,
								const std::string& cgiRoot) {
	Response response;
	this->_scriptPath = scriptPath;
	this->_workingDir = cgiRoot;

	// Setup environment variables
	setupEnvironment(request, scriptPath);

	int inputPipe[2];  // Parent to child
	int outputPipe[2]; // Child to parent

	if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0) {
		response.setStatusCode(500);
		return response;
	}

	pid_t pid = fork();

	if (pid < 0) {
		response.setStatusCode(500);
		return response;
	}

	if (pid == 0) {
		// Child process
		close(inputPipe[1]);   // Close write end of input pipe
		close(outputPipe[0]);  // Close read end of output pipe

		// Redirect stdin to input pipe
		dup2(inputPipe[0], STDIN_FILENO);
		// Redirect stdout to output pipe
		dup2(outputPipe[1], STDOUT_FILENO);

		// Change to working directory
		if (chdir(_workingDir.c_str()) < 0) {
			exit(1);
		}

		// Get CGI executable based on file extension
		std::string cgiExec = getCGIExecutable(scriptPath);
		if (cgiExec.empty()) {
			exit(1);
		}

		// Create environment array
		char** env = createEnvArray();

		// Execute CGI
		char* const args[] = {
				const_cast<char*>(cgiExec.c_str()),
				const_cast<char*>(scriptPath.c_str()),
				NULL
		};

		execve(cgiExec.c_str(), args, env);

		// If execve returns, there was an error
		freeEnvArray(env);
		exit(1);
	}

	// Parent process
	close(inputPipe[0]);  // Close read end of input pipe
	close(outputPipe[1]); // Close write end of output pipe

	// Check for chunked encoding and handle request body
	const std::map<std::string, std::string>& headers = request.getHeaders();
	std::map<std::string, std::string>::const_iterator it =
			headers.find("Transfer-Encoding");

	std::string requestBody = request.getBody();
	if (it != headers.end() && it->second == "chunked") {
		requestBody = unchunkData(requestBody);
	}

	// Write request body to CGI
	writeToPipe(inputPipe[1], requestBody);
	close(inputPipe[1]); // Signal EOF to CGI

	// Read CGI output
	std::string cgiOutput = readFromPipe(outputPipe[0]);
	close(outputPipe[0]);

	// Wait for CGI process to finish
	int status;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		parseScriptOutput(cgiOutput, response);
	} else {
		response.setStatusCode(500);
	}

	return response;
}

void CGIHandler::setupEnvironment(const HTTPRequest& request, const std::string& scriptPath) {
	_environMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	_environMap["SERVER_PROTOCOL"] = request.getVersion();
	_environMap["REQUEST_METHOD"] = request.getMethod();
	_environMap["SCRIPT_FILENAME"] = scriptPath;
	_environMap["PATH_INFO"] = scriptPath;

	// Get query string from the URI
	std::string uri = request.getURI();
	size_t queryPos = uri.find('?');
	if (queryPos != std::string::npos) {
		_environMap["QUERY_STRING"] = uri.substr(queryPos + 1);
	} else {
		_environMap["QUERY_STRING"] = "";
	}

	// Get headers
	const std::map<std::string, std::string>& headers = request.getHeaders();
	std::map<std::string, std::string>::const_iterator it;

	// Set content type and length if available
	it = headers.find("Content-Type");
	if (it != headers.end()) {
		_environMap["CONTENT_TYPE"] = it->second;

		// Handle multipart form data
		if (request.getBoundary().length() > 0) {
			_environMap["CONTENT_TYPE"] += "; boundary=" + request.getBoundary();
		}
	}

	it = headers.find("Content-Length");
	if (it != headers.end()) {
		_environMap["CONTENT_LENGTH"] = it->second;
	}

	// Add remaining headers as HTTP_*
	for (it = headers.begin(); it != headers.end(); ++it) {
		if (it->first != "Content-Type" && it->first != "Content-Length") {
			std::string headerName = it->first;
			std::string envName = "HTTP_";

			// Convert header name to CGI environment variable format
			for (size_t i = 0; i < headerName.length(); ++i) {
				if (headerName[i] == '-') {
					envName += '_';
				} else {
					envName += toupper(headerName[i]);
				}
			}

			_environMap[envName] = it->second;
		}
	}

	_environMap["REDIRECT_STATUS"] = "200";
}

std::string CGIHandler::getCGIExecutable(const std::string& extension) {
	// Example implementation - should be configured in server config
	std::string ext = extension.substr(extension.find_last_of(".") + 1);
	if (ext == "php") return "/usr/bin/php-cgi";
	if (ext == "py") return "/usr/bin/python";
	return "";
}

std::string CGIHandler::unchunkData(const std::string& chunkedData) {
	// Implementation for unchunking data
	// Returns unchunked data
	return ""; // Placeholder
}

std::string CGIHandler::readFromPipe(int fd) {
	std::string result;
	char buffer[4096];
	ssize_t bytesRead;

	while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
		result.append(buffer, bytesRead);
	}

	return result;
}

void CGIHandler::writeToPipe(int fd, const std::string& data) {
	size_t totalWritten = 0;
	while (totalWritten < data.length()) {
		ssize_t written = write(fd, data.c_str() + totalWritten,
								data.length() - totalWritten);
		if (written < 0) {
			if (errno == EINTR) continue;
			break;
		}
		totalWritten += written;
	}
}

void CGIHandler::parseScriptOutput(const std::string& output, Response& response) {
	// Parse headers and body from CGI output
	size_t headerEnd = output.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		response.setStatusCode(502);
		return;
	}

	// Parse headers
	std::string headers = output.substr(0, headerEnd);
	std::string body = output.substr(headerEnd + 4);

	// Set response headers and body
	// Implementation details here
}
