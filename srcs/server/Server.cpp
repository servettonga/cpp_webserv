/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/14 09:29:49 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "../utils/Utils.hpp"
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <fcntl.h>
#include <csignal>
#include <sstream>
#include <fstream>

using namespace Utils;

Server::Server(int port, const std::string &host) : _masterSet(), _readSet(), _writeSet() {
	_serverSocket = -1;
	_isRunning = false;
	_port = port;
	_host = host;
	_masterSet = fd_set();
	_readSet = fd_set();
	_writeSet = fd_set();
	_clients = std::map<int, ClientState>();
	_maxFd = 0;
}

Server::~Server() {
	if (_isRunning)
		stop();
}

void Server::start() {
	if (!initializeSocket())
		throw std::runtime_error("Failed to initialize socket");
	_isRunning = true;
	std::cout << "Server " << _host <<" started on port: " << _port << std::endl;

	while (_isRunning) {
		// Copy master sets to working sets
		fd_set readSet = _masterSet;
		fd_set writeSet = _masterSet;
		timeval timeout = {0, 500000}; // 0.5 seconds
		// Call select() with all sets (read, write, except) and timeout value (0.5 seconds) to wait for events
		int ret = select(_maxFd + 1, &readSet, &writeSet, NULL, &timeout);
		if (ret < 0) {
			if (errno != EINTR) // Ignore interrupt
				throw std::runtime_error("Failed to select: " + std::string(strerror(errno)));
			continue;
		}
		// For each file descriptor in the sets (0 to maxFd) check if readable or writable and handle accordingly
		try {
			for (int i = 0; i <= _maxFd; i++) {
				if (FD_ISSET(i, &readSet)) { // Check if readable
					if (i == _serverSocket)
						handleNewConnection();
					else
						handleClientData(i);
				}
				if (FD_ISSET(i, &writeSet)) // Check if writable
					handleClientWrite(i);
			}
		} catch (const std::exception &e) {
			std::cerr << "Error: " << e.what() << std::endl;
		}
	}
}

void Server::stop() {
	_isRunning = false;
	for (std::map<int, ClientState>::iterator it = _clients.begin(); it != _clients.end(); it++)
		close(it->first);
	_clients.clear();
	close(_serverSocket);
	FD_ZERO(&_masterSet);
	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
	std::cout << "Server " << _host << " stopped" << std::endl;
}

// ********** Socket setup methods **********

bool Server::initializeSocket() {
	// Create socket (IPv4, TCP)
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0)
		return (std::cerr << "Error: socket creation failed: " << strerror(errno) << std::endl, false);

	// Set socket options (reuse address)
	int opt = 1; // Enable address reuse
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		return (std::cerr << "Error: setsockopt failed: " << strerror(errno) << std::endl, false);

	// Set non-blocking mode
	setNonBlocking(_serverSocket);
	sockaddr_in addr = {};
	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(_port); // Port
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // Any address

	// Bind to specified port and address
	if (bind(_serverSocket, (sockaddr *)&addr, sizeof(addr)) < 0)
		return (std::cerr << "Error: bind failed: " << strerror(errno) << std::endl, false);

	// Listen for connections
	if (listen(_serverSocket, 10) < 0)
		return (std::cerr << "Error: listen failed: " << strerror(errno) << std::endl, false);

	// Clear all entries in the sets
	FD_ZERO(&_masterSet);
	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
	// Add server socket to the master set
	FD_SET(_serverSocket, &_masterSet);
	_maxFd = _serverSocket;

	return true;
}

void Server::setNonBlocking(int sockfd) {
	int flags = fcntl(sockfd, F_GETFL, 0); // Get current flags
	if (flags < 0)
		throw std::runtime_error("Failed to get socket flags");
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) // Set non-blocking mode
		throw std::runtime_error("Failed to set socket to non-blocking mode");
}

// ********** Connection handling methods **********

void Server::handleNewConnection() {
	sockaddr_in addr = {}; // Client address info will be stored here
	socklen_t addrLen = sizeof(addr);
	int clientFd = accept(_serverSocket, (sockaddr *)&addr, &addrLen); // Accept new connection
	if (clientFd < 0) { // Error
		if (errno != EWOULDBLOCK && errno != EAGAIN) // Ignore would block
			throw std::runtime_error("Failed to accept new connection: " + std::string(strerror(errno)));
		return ;
	}
	setNonBlocking(clientFd); // Set new socket to non-blocking
	FD_SET(clientFd, &_masterSet); // Add to the master set
	_maxFd = std::max(_maxFd, clientFd); // Update maxFd if needed
	ClientState client = { "", "", false, false }; // Initialize client state
	_clients[clientFd] = client;
}

void Server::handleClientData(int clientFd) {
	/*
		handleClientData(clientFd):
		1. Create read buffer
		2. Receive data from client
		3. IF error or connection closed:
		   Close connection
		4. Append data to client's request buffer
		5. IF request complete:
		   Process request
	*/
	char buffer[1024] = {}; // Read buffer (1KB) - can be increased if needed
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0); // Receive data from client
	if (bytesRead <= 0) {
		if (bytesRead < 0 && errno != EWOULDBLOCK && errno != EAGAIN) // Ignore would block and try again
			throw std::runtime_error("Failed to receive data: " + std::string(strerror(errno)));
		closeConnection(clientFd);
		return ;
	}
	ClientState &client = _clients[clientFd];
	client.requestBuffer += std::string(buffer, bytesRead);
	// If request complete, process it
	if (!client.requestComplete && isRequestComplete(client.requestBuffer)) {
		processRequest(clientFd, _clients[clientFd]);
		client.requestComplete = true;
	}
}

void Server::handleClientWrite(int clientFd) {
	/*
		handleClientWrite(clientFd):
		1. Get client state
		2. Send available response data
		3. IF error:
		   Handle error (except would block)
		4. Remove sent data from buffer
		5. IF buffer empty:
		   IF HTTP/1.0 or Connection: close:
			 Close connection
		   ELSE:
			 Reset for next request
	*/
	ClientState &client = _clients[clientFd];
	if (client.responseBuffer.empty()) // Nothing to send
		return ;

	size_t sendSize = std::min(client.responseBuffer.size(), static_cast<size_t>(4096)); // Send up to 4096 bytes
	ssize_t bytesSent = send(clientFd, client.responseBuffer.c_str(), sendSize, 0);
	if (bytesSent < 0) { // Error
		if (errno != EWOULDBLOCK && errno != EAGAIN) // Ignore would block
			closeConnection(clientFd);
		return ;
	}
	// Remove sent data from buffer
	if (bytesSent > 0)
		client.responseBuffer.erase(0, bytesSent); // Remove sent data from buffer

	// If buffer empty and connection should close, close connection or reset for the next request (keep-alive)
	if (client.responseBuffer.empty()) {
		// HTTP/1.0 - close connection by default, unless Connection: keep-alive header present
		if (client.requestBuffer.find("HTTP/1.0") != std::string::npos || client.requestBuffer.find("Connection: close") != std::string::npos)
			closeConnection(clientFd);
		else {
			// Reset for the next request
			client.requestBuffer.clear();
			client.requestComplete = false;
		}
	}
}

// Clean up client connection
void Server::closeConnection(int clientFd) {
	close(clientFd);
	FD_CLR(clientFd, &_masterSet);
	_clients.erase(clientFd);
}

// ********** Request/Response methods **********

void Server::processRequest(int clientFd, Server::ClientState &client) {
	/*
		processRequest(clientFd, client):
		1. Parse request line (method, path, version)
		2. Parse headers
		3. IF POST:
		   Extract body using Content-Length
		4. Create response object
		5. SWITCH on method:
		   CASE GET:
			 Handle GET request
		   CASE POST:
			 Handle POST request
		   DEFAULT:
			 Send 405 Method Not Allowed
		6. Set response in client buffer
		7. Clear request buffer
	*/
	if (client.requestBuffer.size() > 8192) { // Request too large (8KB limit)
		sendErrorResponse(clientFd, 413, "Payload Too Large");
		return ;
	}
	size_t pos = client.requestBuffer.find("\r\n\r\n");
	if (pos == std::string::npos) { // Invalid request
		sendErrorResponse(clientFd, 400, "Bad Request");
		return ;
	}
	// Parse request
	std::istringstream iss(client.requestBuffer); // Create input stream from request buffer
	std::string requestLine;
	std::getline(iss, requestLine); // Get the first line of the request

	// Parse request line
	std::string method, path, version;
	std::istringstream issLine(requestLine);
	issLine >> method >> path >> version; // Create input stream from request line

	// Create response object
	Response response(200); // Default to 200 OK

	// Set connection header based on version and request headers
	if (version == "HTTP/1.1") {
		if (client.requestBuffer.find("Connection: close") != std::string::npos) {
			response.addHeader("Connection", "close");
		} else {
			response.addHeader("Connection", "keep-alive");
		}
	} else if (version == "HTTP/1.0") {
		if (client.requestBuffer.find("Connection: keep-alive") != std::string::npos) {
			response.addHeader("Connection", "keep-alive");
		} else {
			response.addHeader("Connection", "close");
		}
	} else {
		sendErrorResponse(clientFd, 505, "HTTP Version Not Supported");
		return;
	}
	response.addHeader("Server", _host);

	// Parse headers
	std::map<std::string, std::string> headers;
	std::string headerLine;
	while (std::getline(iss, headerLine) && headerLine != "\r") { // Read headers until empty line
		pos = headerLine.find(": ");
		if (pos != std::string::npos) {
			std::string key = headerLine.substr(0, pos);
			std::string value = headerLine.substr(pos + 2); // Skip ": "
			if (!value.empty() && value[value.size() - 1] == '\r') // Remove trailing \r
				value = value.substr(0, value.size() - 1);
			headers[key] = value;
		}
	}

	// Route request based on method
	if (method == "GET")
		handleGET(path, headers, response);
	else if (method == "POST") {
		// Extract body using Content-Length
		size_t contentLength = 0;
		if (headers.find("Content-Length") != headers.end()) // Content-Length header present
			std::stringstream(headers["Content-Length"]) >> contentLength;
		if (client.requestBuffer.size() >= static_cast<std::streamoff>(iss.tellg()) + contentLength) { // Body present
			std::string body = client.requestBuffer.substr(iss.tellg(), contentLength);
			handlePOST(path, headers, body, response);
		} else {
			sendErrorResponse(clientFd, 411, "Length Required"); // Content-Length missing or invalid
			return ;
		}
	} else {
		response.setStatusCode(405); // Method Not Allowed
		response.addHeader("Content-Type", "text/plain");
		response.setBody("Method Not Allowed");
	}

	response.updateContentLength();
	client.responseBuffer = response.toString(); // Set response in client buffer
}

void Server::sendResponse(int clientFd, Server::ClientState &client) {
	/*
		sendResponse(clientFd, client):
		1. IF response buffer empty:
		   Return
		2. Send available data
		3. IF error:
		   Handle error (except would block)
		4. Remove sent data from buffer
		5. IF buffer empty:
		   IF connection should close:
			 Close connection
	*/
	(void)clientFd;
	(void)client;
}

void Server::sendErrorResponse(int clientFd, int statusCode, const std::string &message) {
	/*
		sendErrorResponse(clientFd, statusCode, message):
		1. Create response object
		2. Set status code from parameter
		3. Add Content-Type: text/html header
		4. Create a simple HTML error page with message
		5. Set response body
		6. Convert response to string
		7. Set client's response buffer
	*/
	Response response(statusCode);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>" + message + "</h1></body></html>");
	std::string responseStr = response.toString();
	_clients[clientFd].responseBuffer = responseStr;
}


// ********** HTTP method handlers **********

void Server::handleGET(const std::string &path, const std::map<std::string, std::string> &headers, Response &response) {
	/*
		handleGET(path, headers, response):
		1. IF path is root ("/"):
		   - Set 200 OK status
		   - Set Content-Type: text/html
		   - Set welcome page as body
		   RETURN

		2. Try to resolve the file path:
		   IF the path contains ".." or illegal characters:
			 Set 403 Forbidden
			 RETURN

		3. IF the file exists:
		   - Get file extension
		   - Set appropriate Content-Type header
		   - Set 200 OK status
		   - Read file content
		   - Set file content as body
		   RETURN

		4. ELSE:
		   Set 404 Not Found
		   RETURN
	*/
	(void)headers; // Unused for now

	// Validate the path to prevent directory traversal
	if (path.find("..") != std::string::npos || path.find('~') != std::string::npos)
		return (response.setStatusCode(403), response.setBody("Forbidden"), void());

	// If the path is root, return welcome page
	if (path == "/") {
		response.setStatusCode(200);
		response.addHeader("Content-Type", "text/html");
		response.addHeader("Content-Length", StringUtils::numToString(response.getBody().size()));
		response.setBody("<html><body><h1>Welcome to the server!</h1></body></html>");
		return ;
	}

	// Try to open the file
	std::string fullPath = "www" + path;
	std::ifstream file(fullPath.c_str(), std::ios::binary); // Open file in binary mode
	if (file.is_open()) {
		// Get file extension
		std::string extension = fullPath.substr(fullPath.find_last_of('.') + 1);
		// Set Content-Type based on file extension (later move to HTTPUtils)
		if (extension == "html")
			response.addHeader("Content-Type", "text/html");
		else if (extension == "css")
			response.addHeader("Content-Type", "text/css");
		else if (extension == "js")
			response.addHeader("Content-Type", "application/javascript");
		else if (extension == "jpg" || extension == "jpeg")
			response.addHeader("Content-Type", "image/jpeg");
		else if (extension == "png")
			response.addHeader("Content-Type", "image/png");
		else if (extension == "gif")
			response.addHeader("Content-Type", "image/gif");
		else if (extension == "ico")
			response.addHeader("Content-Type", "image/x-icon");
		else
			response.addHeader("Content-Type", "application/octet-stream");

		// Read file content
		std::stringstream buffer;
		buffer << file.rdbuf();
		response.setStatusCode(200);
		response.setBody(buffer.str());
		file.close();
	} else {
		response.setStatusCode(404);
		response.setBody("Not Found");
	}
}

void Server::handlePOST(const std::string &path, const std::map<std::string, std::string> &headers, const std::string &body,
				   Response &response) {
	/*
	handlePOST(path, headers, body, response):
	1. IF Content-Type header missing:
	   Set 400 Bad Request
	   RETURN

	2. IF Content-Length header invalid:
	   Set 411 Length Required
	   RETURN

	3. IF body size exceeds max allowed:
	   Set 413 Payload Too Large
	   RETURN

	4. Process POST based on the path:
	   IF the path not handled:
		 Set 404 Not Found
	   ELSE:
		 Handle request data
		 Set appropriate status code
		 Set response headers
		 Set response body
	*/
	(void)path; // Unused for now
	// Validate Content-Type header
	if (headers.find("Content-Type") == headers.end())
		return (response.setStatusCode(400));

	// Echo back the received data for now
	response.setStatusCode(200);
	response.addHeader("Content-Type", "text/plain");
	response.setBody(body);
}

// ********** Helper methods **********

// Update the highest file descriptor
void Server::updateMaxFd() {

}

// Check if full request received
bool Server::isRequestComplete(const std::string &request) {
	return (request.find("\r\n\r\n") != std::string::npos);
}
