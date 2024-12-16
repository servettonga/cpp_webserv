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

	// Set socket buffer sizes
	int rcvBufSize = 256 * 1024;  // 256KB
	int sndBufSize = 256 * 1024;  // 256KB
	setsockopt(_serverSocket, SOL_SOCKET, SO_RCVBUF, &rcvBufSize, sizeof(rcvBufSize));
	setsockopt(_serverSocket, SOL_SOCKET, SO_SNDBUF, &sndBufSize, sizeof(sndBufSize));

	// Set TCP options
	int tcpQuickAck = 1;
	int tcpNoDelay = 1;
	setsockopt(_serverSocket, IPPROTO_TCP, TCP_QUICKACK, &tcpQuickAck, sizeof(tcpQuickAck));
	setsockopt(_serverSocket, IPPROTO_TCP, TCP_NODELAY, &tcpNoDelay, sizeof(tcpNoDelay));

	// Set keep-alive
	int keepAlive = 1;
	int keepIdle = 60;
	int keepInterval = 10;
	int keepCount = 3;
	setsockopt(_serverSocket, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
	setsockopt(_serverSocket, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle));
	setsockopt(_serverSocket, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval));
	setsockopt(_serverSocket, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));

	// Set non-blocking
	setNonBlocking(_serverSocket);

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

	if (bytesRead > 0) {
		client.requestBuffer.append(buffer, bytesRead);
		client.lastActivity = time(NULL);

		// Check if we have headers
		size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
		if (headerEnd != std::string::npos) {
			// Parse headers to get Content-Length
			if (client.contentLength == 0) {
				std::string headers = client.requestBuffer.substr(0, headerEnd);
				size_t clPos = headers.find("Content-Length: ");
				if (clPos != std::string::npos) {
					size_t clEnd = headers.find("\r\n", clPos);
					std::string clStr = headers.substr(clPos + 16, clEnd - (clPos + 16));
					client.contentLength = std::atol(clStr.c_str());
				}
			}

			// Handle Expect: 100-continue
			if (!client.continueSent &&
				client.requestBuffer.find("Expect: 100-continue") != std::string::npos) {
				std::string continueResponse = "HTTP/1.1 100 Continue\r\n\r\n";
				send(clientFd, continueResponse.c_str(), continueResponse.length(), MSG_DONTWAIT);
				client.continueSent = true;
				return;  // Wait for the body
			}

			// Check if we have the complete body
			size_t totalLength = headerEnd + 4 + client.contentLength;
			if (client.requestBuffer.length() >= totalLength) {
				processCompleteRequests(clientFd, client);
			}
		}
	} else if (bytesRead == 0) {
		closeConnection(clientFd);
	} else if (errno != EAGAIN && errno != EWOULDBLOCK) {
		closeConnection(clientFd);
	}
}

void Server::processCompleteRequests(int clientFd, ClientState &client) {
	// Process headers
	size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return;  // Need more headers

	// Check for chunked transfer
	std::string transferEncoding;
	size_t tePos = client.requestBuffer.find("Transfer-Encoding: ");
	if (tePos != std::string::npos && tePos < headerEnd) {
		size_t teEnd = client.requestBuffer.find("\r\n", tePos);
		transferEncoding = client.requestBuffer.substr(tePos + 19, teEnd - (tePos + 19));
	}

	// Handle chunked data
	if (transferEncoding == "chunked") {
		size_t endPos = client.requestBuffer.find("0\r\n\r\n");
		if (endPos == std::string::npos)
			return;  // Need more chunks
	}

	// Process request
	HTTPRequest request;
	if (!request.parse(client.requestBuffer)) {
		sendBadRequestResponse(clientFd);
		client.state = WRITING_RESPONSE;
		client.requestBuffer.clear();
		return;
	}

	// Handle request
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

	const size_t CHUNK_SIZE = 8192;
	size_t toWrite = std::min(client.responseBuffer.size(), CHUNK_SIZE);

	ssize_t sent = send(clientFd,
						client.responseBuffer.c_str(),
						toWrite,
						MSG_NOSIGNAL);

	if (sent > 0) {
		client.responseBuffer.erase(0, sent);
		client.lastActivity = time(NULL);

		if (client.responseBuffer.empty()) {
			// Reset for next request
			client.state = IDLE;
			client.requestBuffer.clear();
			client.contentLength = 0;
			client.continueSent = false;

			// Check connection status
			bool shouldClose = false;
			shouldClose |= (client.requestBuffer.find("HTTP/1.0") != std::string::npos);
			shouldClose |= (client.requestBuffer.find("Connection: close") != std::string::npos);
			shouldClose |= ((time(NULL) - client.lastActivity) > KEEP_ALIVE_TIMEOUT);

			if (shouldClose)
				closeConnection(clientFd);
		}
	} else if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		_logger.error("Write error: " + std::string(strerror(errno)));
		closeConnection(clientFd);
	}
}

void Server::sendBadRequestResponse(int clientFd) {
	Response response(400);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>Bad Request</h1></body></html>");
	_clients[clientFd].responseBuffer = response.toString();
}

void Server::closeConnection(int clientFd) {
	if (clientFd < 0)
		return;
	try {
		shutdown(clientFd, SHUT_RDWR);
	} catch (...) {
		// Ignore shutdown errors
	}
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
