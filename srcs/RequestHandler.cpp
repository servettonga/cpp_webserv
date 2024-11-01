/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 20:14:45 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>

RequestHandler::RequestHandler(const std::vector<ServerConfig>& configs) : serverConfigs(configs) {
	// Constructor implementation
}

RequestHandler::~RequestHandler() {
	// Destructor implementation
}

// Parses the request and dispatches it based on the HTTP method.
void RequestHandler::handleRequest(const std::string& requestStr, int clientFd) {
	try {
		HTTPRequest request(requestStr);

		// Find the appropriate server configuration
		const ServerConfig& serverConfig = findServerConfig(request);

		// Find the matching location configuration
		const LocationConfig& locationConfig = findLocationConfig(request, serverConfig);

		// Check if the method is allowed
		if (!isMethodAllowed(request.getMethod(), locationConfig)) {
			sendErrorResponse(clientFd, 405, serverConfig);
			return;
		}

		// Dispatch the request based on the method
		const std::string& method = request.getMethod();
		if (method == "GET") {
			handleGetRequest(request, clientFd, serverConfig, locationConfig);
		} else if (method == "POST") {
			handlePostRequest(request, clientFd, serverConfig, locationConfig);
		} else if (method == "DELETE") {
			handleDeleteRequest(request, clientFd, serverConfig, locationConfig);
		} else {
			sendErrorResponse(clientFd, 501, serverConfig);
		}
	} catch (const std::exception& e) {
		// Log the error
		std::cerr << "Error handling request: " << e.what() << std::endl;
		// Send 500 Internal Server Error
		// Assuming that we can get a serverConfig for error handling
		// Here, we'll just use the first serverConfig as a fallback
		if (!serverConfigs.empty()) {
			sendErrorResponse(clientFd, 500, serverConfigs[0]);
		}
	}
}

// Selects the appropriate ServerConfig based on the request
const ServerConfig& RequestHandler::findServerConfig(const HTTPRequest& request) {
	// TODO: Implement logic to select the appropriate ServerConfig based on the request
	// Placeholder implementation returns the first ServerConfig
	return serverConfigs[0];
}

// Selects the appropriate LocationConfig based on the request URI
const LocationConfig& RequestHandler::findLocationConfig(const HTTPRequest& request, const ServerConfig& serverConfig) {
	// TODO: Implement logic to select the appropriate LocationConfig based on the request URI
	// Placeholder implementation returns the first LocationConfig
	if (serverConfig.locations.empty()) {
		throw std::runtime_error("No location configurations available");
	}
	return serverConfig.locations[0];
}

// Checks if the HTTP method is allowed for the requested location
bool RequestHandler::isMethodAllowed(const std::string& method, const LocationConfig& locationConfig) {
	// Check if the method is in the list of allowed methods for the location
	const std::vector<std::string>& allowedMethods = locationConfig.allowedMethods;
	return std::find(allowedMethods.begin(), allowedMethods.end(), method) != allowedMethods.end();
}

// Processes GET requests, serves files from the server
void RequestHandler::handleGetRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& locationConfig) {
	// Construct the file path based on the URI and the location root
	std::string uri = sanitizeURI(request.getURI());
	uri = urlDecode(uri);
	std::string filePath = constructFilePath(uri, locationConfig);

	// Check if the file exists and is a regular file
	struct stat fileStat = {};
	if (stat(filePath.c_str(), &fileStat) == -1 || !S_ISREG(fileStat.st_mode)) {
		sendErrorResponse(clientFd, 404, serverConfig);
		return;
	}

	// Read the file content
	std::string fileContent = readFileContent(filePath);

	// Create the response
	Response response;
	response.setStatusCode(200);
	response.setHeader("Content-Type", "text/html"); // You may want to detect the MIME type
	std::stringstream ss;
	ss << fileContent.size();
	response.setHeader("Content-Length", ss.str());
	response.setBody(fileContent);

	// Send the response
	std::string responseStr = response.toString();
	send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}

// Processes POST requests; placeholder implementation provided
void RequestHandler::handlePostRequest(const HTTPRequest &request, int clientFd, const ServerConfig &serverConfig, const LocationConfig &locationConfig) {
	Response response;
	response.setStatusCode(200);
	response.setHeader("Content-Length", "0");

	// Send the response
	std::string responseStr = response.toString();
	send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}

// Processes DELETE requests by attempting to delete the specified resource
void RequestHandler::handleDeleteRequest(const HTTPRequest &request, int clientFd, const ServerConfig &serverConfig, const LocationConfig &locationConfig) {
	std::string uri = sanitizeURI(request.getURI());
	uri = urlDecode(uri);
	std::string filePath = constructFilePath(uri, locationConfig);

	if (remove(filePath.c_str()) == 0) {
		Response response;
		response.setStatusCode(200);
		response.setHeader("Content-Length", "0");

		std::string responseStr = response.toString();
		send(clientFd, responseStr.c_str(), responseStr.size(), 0);
	} else {
		sendErrorResponse(clientFd, 403, serverConfig);
	}
}

// Sends an HTTP error response with the appropriate status code and message
// Custom error pages are utilized if configured in ServerConfig
void RequestHandler::sendErrorResponse(int clientFd, int statusCode, const ServerConfig& serverConfig) {
	Response response;
	response.setStatusCode(statusCode);

	// Check for custom error page
	std::map<int, std::string>::const_iterator it = serverConfig.errorPages.find(statusCode);
	if (it != serverConfig.errorPages.end()) {
		std::string errorPagePath = it->second;
		std::string errorPageContent = readFileContent(errorPagePath);
		if (!errorPageContent.empty()) {
			response.setHeader("Content-Type", "text/html");
			std::stringstream ss;
			ss << errorPageContent.size();
			response.setHeader("Content-Length", ss.str());
			response.setBody(errorPageContent);
		} else {
			// Use default error message
			std::string defaultMessage = "<html><body><h1>" + intToString(statusCode) + " " + getReasonPhrase(statusCode) + "</h1></body></html>";
			response.setHeader("Content-Type", "text/html");
			std::stringstream ss;
			ss << defaultMessage.size();
			response.setHeader("Content-Length", ss.str());
			response.setBody(defaultMessage);
		}
	} else {
		// Use default error message
		std::string defaultMessage = "<html><body><h1>" + intToString(statusCode) + " " + getReasonPhrase(statusCode) + "</h1></body></html>";
		response.setHeader("Content-Type", "text/html");
		std::stringstream ss;
		ss << defaultMessage.size();
		response.setHeader("Content-Length", ss.str());
		response.setBody(defaultMessage);
	}

	// Send the response
	std::string responseStr = response.toString();
	send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}

// Helper function for C++98 compatibility
std::string RequestHandler::intToString(int value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

// Returns the standard reason phrase corresponding to an HTTP status code
std::string RequestHandler::getReasonPhrase(int statusCode) {
	// Return the reason phrase corresponding to the status code
	switch (statusCode) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		default: return "Unknown Status";
	}
}

// Reads the content of a file into a string
std::string RequestHandler::readFileContent(const std::string& filePath) {
	// Read the content of the file specified by filePath
	std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		// Failed to open the file
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

// Constructs the path to the requested file based on the root directory and URI
std::string RequestHandler::constructFilePath(const std::string& uri, const LocationConfig& locationConfig) {
	// Construct the file path by combining the root and the URI
	std::string root = locationConfig.root;
	if (root.empty()) {
		root = "."; // Default root directory
	}
	// Ensure there's a '/' between root and uri
	if (root.empty() || root[root.length() - 1] != '/') {
		root += '/';
	}
	std::string filePath = root + uri;
	return filePath;
}

// Cleans up the URI by removing query parameters and fragments
std::string RequestHandler::sanitizeURI(const std::string& uri) {
	// Remove query string and fragment identifier
	size_t pos = uri.find_first_of("?#");
	if (pos != std::string::npos) {
		return uri.substr(0, pos);
	}
	return uri;
}

// Decodes URL-encoded strings (e.g., converting %20 to spaces)
std::string RequestHandler::urlDecode(const std::string& str) {
	std::string decoded;
	char h1, h2;
	for (size_t i = 0; i < str.length(); ++i) {
		if (str[i] == '%' && i + 2 < str.length() &&
			isxdigit(h1 = str[i + 1]) && isxdigit(h2 = str[i + 2])) {
			h1 = std::toupper(h1);
			h2 = std::toupper(h2);
			h1 = h1 >= 'A' ? h1 - 'A' + 10 : h1 - '0';
			h2 = h2 >= 'A' ? h2 - 'A' + 10 : h2 - '0';
			decoded += static_cast<char>(h1 * 16 + h2);
			i += 2;
		} else if (str[i] == '+') {
			decoded += ' ';
		} else {
			decoded += str[i];
		}
	}
	return decoded;
}

/*
 	TODO: Handling Static Files and Directory Listing
	Implement functionality to serve static files and generate directory listings if enabled.

	Serving Static Files:
	- Check if the requested URI corresponds to a file on the server.
	- Read the file content and send it in the HTTP response.
	- Set the appropriate Content-Type based on the file extension.

 	Directory Listing:
	- If the URI corresponds to a directory and directory listing is enabled, generate an HTML page listing the contents of the directory.
	- If a default file (e.g., index.html) exists in the directory, serve that file instead.
 */

/*
	TODO: Implementing CGI Handling
	Support executing CGI scripts based on file extensions.

	Steps:
	- Detect if the requested resource matches a CGI route.
	- Use fork() and execve() to execute the CGI script.
	- Set up environment variables required by the CGI.
	- Communicate with the CGI process using pipes.
 */

/*
	TODO: Handling File Uploads
	Support file uploads via the POST method.

	Steps:
	- Parse multipart form data in the POST request.
	- Extract file contents and save them to the designated upload directory.
	- Send an appropriate response indicating success or failure.
 */
