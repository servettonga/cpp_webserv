/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/15 00:15:55 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "../handlers/RequestHandler.hpp"
#include "../utils/Utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>

Logger &Server::_logger = Logger::getInstance();

Server::Server(const ServerConfig &config) :
		_host(config.host),
		_port(config.port),
		_serverSocket(-1),
		_config(config),
		_maxFd(0) {
	_logger.configure("logs/server.log", INFO, true, true);
	_config.precomputePaths();
}

Server::~Server() {
	stop();
	std::cout << "Server " << _host << " stopped" << std::endl;
}

bool Server::initializeSocket() {
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0)
		return false;
	// Set socket options
	int opt = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0 ||
		setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
		close(_serverSocket);
		return false;
	}
	// Set socket to non-blocking
	setNonBlocking(_serverSocket);
	// Set TCP options for better performance
	int tcpQuickAck = 1;
	setsockopt(_serverSocket, IPPROTO_TCP, TCP_QUICKACK, &tcpQuickAck, sizeof(tcpQuickAck));
	int tcpNoDelay = 1;
	setsockopt(_serverSocket, IPPROTO_TCP, TCP_NODELAY, &tcpNoDelay, sizeof(tcpNoDelay));
	// Set keep-alive
	int keepAlive = 1;
	setsockopt(_serverSocket, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
	// Increase socket buffers
	int rcvBuf = 256 * 1024; // 256KB
	int sndBuf = 256 * 1024; // 256KB
	setsockopt(_serverSocket, SOL_SOCKET, SO_RCVBUF, &rcvBuf, sizeof(rcvBuf));
	setsockopt(_serverSocket, SOL_SOCKET, SO_SNDBUF, &sndBuf, sizeof(sndBuf));
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(_serverSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		close(_serverSocket);
		return false;
	}
	// Increase backlog for high concurrency
	if (listen(_serverSocket, SOMAXCONN) < 0) {
		close(_serverSocket);
		return false;
	}
	return true;
}

void Server::setNonBlocking(int sockfd) {
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0)
		throw std::runtime_error("Failed to get socket flags");
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error("Failed to set socket to non-blocking mode");
}

void Server::handleClientData(int clientFd) {
	ClientState &client = _clients[clientFd];

	if (client.state == WRITING_RESPONSE)
		return;
	char buffer[BUFFER_SIZE];
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), MSG_DONTWAIT);
	if (bytesRead < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			closeConnection(clientFd);
		return;
	}
	if (bytesRead == 0) {
		closeConnection(clientFd);
		return;
	}
	client.state = READING_REQUEST;
	client.lastActivity = time(NULL);
	client.requestBuffer.append(buffer, bytesRead);
	// Process complete requests if available
	processCompleteRequests(clientFd, client);
}

void Server::processCompleteRequests(int clientFd, ClientState &client) {
	size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return;
	if (client.requestBuffer.find("HTTP/1.1") == std::string::npos) {
		sendBadRequestResponse(clientFd);
		closeConnection(clientFd);
		return;
	}
	size_t contentLengthPos = client.requestBuffer.find("Content-Length: ");
	if (contentLengthPos != std::string::npos) {
		size_t endOfLength = client.requestBuffer.find("\r\n", contentLengthPos);
		size_t length = std::atol(client.requestBuffer.substr(
				contentLengthPos + 16, endOfLength - (contentLengthPos + 16)).c_str());
		// Wait for complete body
		if (client.requestBuffer.length() < headerEnd + 4 + length)
			return;
	}
	HTTPRequest request;
	if (!request.parse(client.requestBuffer)) {
		sendBadRequestResponse(clientFd);
		client.state = WRITING_RESPONSE;
		client.requestBuffer.clear();
		return;
	}
	RequestHandler handler(_config);
	Response response = handler.handleRequest(request);
	client.responseBuffer = response.toString();
	client.state = WRITING_RESPONSE;
	client.requestBuffer.clear();
}

void Server::handleNewConnection() {
	struct sockaddr_in addr = {};
	socklen_t addrLen = sizeof(addr);

	if (_clients.size() >= MAX_CLIENTS) {
		// Accept and immediately close if too many connections
		int tempFd = accept(_serverSocket, (struct sockaddr*)&addr, &addrLen);
		if (tempFd >= 0) {
			close(tempFd);
			_logger.warn("Max clients reached, connection rejected");
		}
		return;
	}
	int clientFd = accept(_serverSocket, (struct sockaddr*)&addr, &addrLen);
	if (clientFd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			_logger.error("Failed to accept connection: " + std::string(strerror(errno)));
		return;
	}
	// Set new socket options
	setNonBlocking(clientFd);
	int keepAlive = 1;
	setsockopt(clientFd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
	// Initialize client state
	_clients[clientFd] = ClientState();
	updateMaxFileDescriptor();
}

void Server::handleClientWrite(int clientFd) {
	ClientState &client = _clients[clientFd];

	if (client.state != WRITING_RESPONSE || client.responseBuffer.empty())
		return;
	ssize_t sent = send(clientFd,
						client.responseBuffer.c_str(),
						client.responseBuffer.size(),
						MSG_DONTWAIT);
	if (sent > 0) {
		client.responseBuffer.erase(0, sent);
		client.lastActivity = time(NULL);
		if (client.responseBuffer.empty()) {
			if (shouldCloseConnection(client)) {
				closeConnection(clientFd);
			} else {
				client.state = IDLE;
				client.requestBuffer.clear();
			}
		}
	} else if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		closeConnection(clientFd);
	}
}

bool Server::shouldCloseConnection(const ClientState &client) const {
	if (client.requestBuffer.find("HTTP/1.1") == std::string::npos)
		return true;
	if (client.requestBuffer.find("Connection: close") != std::string::npos)
		return true;
	if ((time(NULL) - client.lastActivity) > KEEP_ALIVE_TIMEOUT)
		return true;
	return false;
}

void Server::sendBadRequestResponse(int clientFd) {
	Response response(400);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>Bad Request</h1></body></html>");
	_clients[clientFd].responseBuffer = response.toString();
}

void Server::closeConnection(int clientFd) {
	shutdown(clientFd, SHUT_RDWR);
	close(clientFd);
	_clients.erase(clientFd);
	updateMaxFileDescriptor();
}

void Server::stop() {
	for (std::map<int, ClientState>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		shutdown(it->first, SHUT_RDWR);
		close(it->first);
	}
	_clients.clear();
	if (_serverSocket >= 0) {
		shutdown(_serverSocket, SHUT_RDWR);
		close(_serverSocket);
		_serverSocket = -1;
	}
	_maxFd = 0;
}

void Server::initialize() {
	if (!initializeSocket()) {
		_logger.error("Failed to initialize socket", "Server");
		throw std::runtime_error("Failed to initialize socket");
	}

	updateMaxFileDescriptor();
	_logger.info("Server " + _host + " started on port: " + Utils::numToString(_port), "Server");
}

void Server::handleExistingConnections(fd_set &readSet, fd_set &writeSet) {
	checkIdleConnections();

	std::vector<int> fdsToCheck;
	for (std::map<int, ClientState>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		fdsToCheck.push_back(it->first);
	}
	for (size_t i = 0; i < fdsToCheck.size(); ++i) {
		int fd = fdsToCheck[i];
		if (_clients.find(fd) != _clients.end()) {
			if (FD_ISSET(fd, &readSet))
				handleClientData(fd);
			if (_clients.find(fd) != _clients.end() && FD_ISSET(fd, &writeSet))
				handleClientWrite(fd);
		}
	}
	updateMaxFileDescriptor();
}

void Server::checkIdleConnections() {
	time_t currentTime = time(NULL);
	std::vector<int> idleConnections;

	for (std::map<int, ClientState>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (isConnectionIdle(currentTime, it->second))
			idleConnections.push_back(it->first);
	for (size_t i = 0; i < idleConnections.size(); ++i)
		closeConnection(idleConnections[i]);
}

bool Server::isConnectionIdle(time_t currentTime, const ClientState &client) const {
	return (currentTime - client.lastActivity) > IDLE_TIMEOUT;
}

void Server::updateMaxFileDescriptor() {
	_maxFd = _serverSocket;
	for (std::map<int, ClientState>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (it->first >= 0 && it->first < FD_SETSIZE)
			_maxFd = std::max(_maxFd, it->first);
}
