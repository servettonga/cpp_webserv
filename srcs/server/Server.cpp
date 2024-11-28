/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/28 12:53:27 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "../handlers/RequestHandler.hpp"
#include <cstring>		// For strerror
#include <iostream>		// For std::cout
#include <sys/socket.h>	// For socket functions
#include <netinet/in.h>	// For sockaddr_in
#include <cerrno>		// For errno
#include <fcntl.h>		// For fcntl
#include <sys/types.h>  // For basic system data types
#include <unistd.h>     // For basic system calls
#include <time.h>       // For time functions like strftime

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
	ClientState client = {"", "", false}; // Initialize client state
	_clients[clientFd] = client;
}

void Server::handleClientData(int clientFd) {
	char buffer[8192] = {};
	ClientState &client = _clients[clientFd];

	// Read data
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0) {
		if (bytesRead < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
			std::cerr << "Error: Failed to receive: " << strerror(errno) << std::endl;
		}
		closeConnection(clientFd);
		return;
	}

	// Debug output
	std::cout << "Received " << bytesRead << " bytes" << std::endl;

	// Append to buffer
	client.requestBuffer.append(buffer, bytesRead);
	std::cout << "Total buffer size: " << client.requestBuffer.size() << std::endl;

	// Process request when we have complete headers
	size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
	if (headerEnd != std::string::npos) {
		// For GET and DELETE requests, process immediately
		if (client.requestBuffer.find("GET") == 0 ||
			client.requestBuffer.find("DELETE") == 0) {
			std::cout << "Processing " <<
					  (client.requestBuffer.find("GET") == 0 ? "GET" : "DELETE")
					  << " request..." << std::endl;
			processRequest(clientFd, client);
			client.requestComplete = true;
			return;
		}

		// For POST requests, check Content-Length
		size_t clPos = client.requestBuffer.find("Content-Length: ");
		if (clPos != std::string::npos && clPos < headerEnd) {
			size_t clEnd = client.requestBuffer.find("\r\n", clPos);
			if (clEnd != std::string::npos) {
				std::string lenStr = client.requestBuffer.substr(clPos + 16, clEnd - (clPos + 16));
				size_t expectedLength = std::atol(lenStr.c_str());
				size_t totalExpectedLength = headerEnd + 4 + expectedLength;

				if (client.requestBuffer.length() >= totalExpectedLength) {
					std::cout << "Processing POST request..." << std::endl;
					processRequest(clientFd, client);
					client.requestComplete = true;
				}
			}
		}
	}
}

void Server::handleClientWrite(int clientFd) {
	ClientState &client = _clients[clientFd];

	if (client.responseBuffer.empty())
		return;
	ssize_t bytesSent = send(clientFd, client.responseBuffer.c_str(),
							 client.responseBuffer.size(), 0);
	if (bytesSent < 0) {
		if (errno != EWOULDBLOCK && errno != EAGAIN)
			closeConnection(clientFd);
		return;
	}
	if (bytesSent > 0)
		client.responseBuffer.erase(0, bytesSent);
	// Close connection if response complete and not keep-alive
	if (client.responseBuffer.empty()) {
		if (!client.requestBuffer.empty() &&
			(client.requestBuffer.find("HTTP/1.0") != std::string::npos ||
			 client.requestBuffer.find("Connection: close") != std::string::npos)) {
			closeConnection(clientFd);
		} else {
			// Reset for next request
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
	HTTPRequest request;
	if (!request.parse(client.requestBuffer))
		return sendErrorResponse(clientFd, 400, "Bad Request");

	RequestHandler handler(_config);
	Response response = handler.handleRequest(request);
	// Set response buffer and mark for writing
	client.responseBuffer = response.toString();
	FD_SET(clientFd, &_writeSet);
}

void Server::sendErrorResponse(int clientFd, int statusCode, const std::string &message) {
	Response response(statusCode);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>" + message + "</h1></body></html>");
	std::string responseStr = response.toString();
	_clients[clientFd].responseBuffer = responseStr;
}

// Check if full request received
bool Server::isRequestComplete(const std::string &request) {
	return (request.find("\r\n\r\n") != std::string::npos);
}

Server::Server(const ServerConfig &config)
		: _host(config.host),
		  _port(config.port),
		  _serverSocket(-1),
		  _isRunning(false),
		  _config(config),
		  _maxFd(0)
{
	FD_ZERO(&_masterSet);
	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
}
