/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/30 17:09:46 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "../handlers/RequestHandler.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>

Server::Server(const ServerConfig &config) :
		_host(config.host),
		_port(config.port),
		_serverSocket(-1),
		_isRunning(false),
		_config(config),
		_maxFd(0) {
	FD_ZERO(&_masterSet);
	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
}

Server::~Server() {
	stop();
	std::cout << "Server " << _host << " stopped" << std::endl;
}

void Server::start() {
	if (!initializeSocket())
		throw std::runtime_error("Failed to initialize socket");

	_isRunning = true;
	std::cout << "Server " << _host << " started on port: " << _port << std::endl;
	runEventLoop();
}

bool Server::initializeSocket() {
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0)
		return false;

	int opt = 1;
	setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	setNonBlocking(_serverSocket);

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(_serverSocket, (sockaddr *) &addr, sizeof(addr)) < 0 ||
		listen(_serverSocket, SOMAXCONN) < 0)
		return false;

	FD_SET(_serverSocket, &_masterSet);
	_maxFd = _serverSocket;
	return true;
}

void Server::setNonBlocking(int sockfd) {
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0)
		throw std::runtime_error("Failed to get socket flags");
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error("Failed to set socket to non-blocking mode");
}

void Server::runEventLoop() {
	int maxFailedSelects = 5;
	int failedSelects = 0;
	_lastActivity = time(NULL);

	while (_isRunning) {
		fd_set readSet = _masterSet;
		fd_set writeSet = _writeSet;
		timeval timeout = {0, 500000};

		int activity = select(_maxFd + 1, &readSet, &writeSet, NULL, &timeout);

		if (activity < 0) {
			if (errno == EINTR)
				continue;

			failedSelects++;
			std::cerr << "Select error: " << strerror(errno) << std::endl;

			if (failedSelects >= maxFailedSelects) {
				_isRunning = false;
				break;
			}
			continue;
		}
		failedSelects = 0;
		if (activity > 0) {
			_lastActivity = time(NULL);  // Update activity timestamp
			handleEvents(readSet, writeSet);
		}
		// Check idle timeout
		if (_clients.empty() &&
			difftime(time(NULL), _lastActivity) > IDLE_TIMEOUT) {
			std::cout << "Server idle timeout reached" << std::endl;
			_isRunning = false;
			break;
		}
	}
}

void Server::handleEvents(fd_set &readSet, fd_set &writeSet) {
	for (int i = 0; i <= _maxFd; i++) {
		if (FD_ISSET(i, &readSet))
			handleReadEvent(i);
		if (FD_ISSET(i, &writeSet))
			handleClientWrite(i);
	}
}

void Server::handleReadEvent(int fd) {
	if (fd == _serverSocket)
		handleNewConnection();
	else
		handleClientData(fd);
}

void Server::handleClientData(int clientFd) {
	char buffer[BUFFER_SIZE] = {};
	ClientState &client = _clients[clientFd];

	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0)
		return (closeConnection(clientFd));
	client.requestBuffer.append(buffer, bytesRead);    // Append new data to existing buffer
	processCompleteRequests(clientFd, client);    // Process complete requests
}

void Server::processCompleteRequests(int clientFd, ClientState &client) {
	size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return;

	// For GET and DELETE requests
	if (client.requestBuffer.find("GET") == 0 ||
		client.requestBuffer.find("DELETE") == 0) {
		processRequest(clientFd, client);
		client.requestBuffer.clear();  // Clear buffer after processing
		return;
	}

	// For POST requests
	size_t clPos = client.requestBuffer.find("Content-Length: ");
	if (clPos != std::string::npos && clPos < headerEnd) {
		size_t clEnd = client.requestBuffer.find("\r\n", clPos);
		if (clEnd != std::string::npos) {
			size_t contentLength = std::atol(
					client.requestBuffer.substr(clPos + 16,
												clEnd - (clPos + 16)).c_str()
			);
			if (client.requestBuffer.length() >= headerEnd + 4 + contentLength) {
				processRequest(clientFd, client);
				client.requestBuffer.clear();  // Clear buffer after processing
			}
		}
	}
}

void Server::handleNewConnection() {
	sockaddr_in addr = {};
	socklen_t addrLen = sizeof(addr);

	int clientFd = accept(_serverSocket, (sockaddr *) &addr, &addrLen);
	if (clientFd < 0)
		return;

	setNonBlocking(clientFd);
	FD_SET(clientFd, &_masterSet);
	_maxFd = std::max(_maxFd, clientFd);
	_clients[clientFd] = ClientState();
}

void Server::handleClientWrite(int clientFd) {
	ClientState &client = _clients[clientFd];
	if (client.responseBuffer.empty())
		return;

	// Fill write buffer
	size_t toWrite = std::min(client.writeBufferSize,
							  client.responseBuffer.size());
	client.writeBuffer.assign(
			client.responseBuffer.begin(),
			client.responseBuffer.begin() + toWrite
	);

	// Send buffered data
	ssize_t sent = send(clientFd, client.writeBuffer.data(),
						toWrite, 0);

	if (sent < 0 && errno != EWOULDBLOCK && errno != EAGAIN)
		return (closeConnection(clientFd));

	if (sent > 0) {
		client.responseBuffer.erase(0, sent);
		if (client.responseBuffer.empty() && shouldCloseConnection(client))
			closeConnection(clientFd);
	}
}

bool Server::shouldCloseConnection(const ClientState &client) const {
	return client.requestBuffer.find("HTTP/1.0") != std::string::npos ||
		   client.requestBuffer.find("Connection: close") != std::string::npos;
}

void Server::processRequest(int clientFd, ClientState &client) {
	HTTPRequest request;
	if (!request.parse(client.requestBuffer))
		return sendBadRequestResponse(clientFd);

	RequestHandler handler(_config);
	Response response = handler.handleRequest(request);
	client.responseBuffer = response.toString();
	FD_SET(clientFd, &_writeSet);
}

void Server::sendBadRequestResponse(int clientFd) {
	Response response(400);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>Bad Request</h1></body></html>");
	_clients[clientFd].responseBuffer = response.toString();
	FD_SET(clientFd, &_writeSet);
}

void Server::closeConnection(int clientFd) {
	close(clientFd);
	FD_CLR(clientFd, &_masterSet);
	FD_CLR(clientFd, &_writeSet);
	_clients.erase(clientFd);
}

void Server::stop() {
	_isRunning = false;
	for (std::map<int, ClientState>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		close(it->first);
	}
	_clients.clear();
	close(_serverSocket);
	FD_ZERO(&_masterSet);
	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
}

Server::ClientState::ClientState() : writeBufferSize(1024 * 1024) { // 1MB write buffer
	writeBuffer.reserve(writeBufferSize);
}
