/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/22 14:19:56 by sehosaf          ###   ########.fr       */
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
	_logger.info("Initializing socket on port " + Utils::numToString(_port));

	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0) {
		_logger.error("Failed to create socket: " + std::string(strerror(errno)));
		return false;
	}

	// Set socket options
	int opt = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		_logger.error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
		close(_serverSocket);
		return false;
	}

	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(_serverSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		_logger.error("Failed to bind: " + std::string(strerror(errno)));
		close(_serverSocket);
		return false;
	}

	if (listen(_serverSocket, SOMAXCONN) < 0) {
		_logger.error("Failed to listen: " + std::string(strerror(errno)));
		close(_serverSocket);
		return false;
	}

	setNonBlocking(_serverSocket);
	_logger.info("Socket initialized successfully");
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

	static const size_t CHUNK_BUFFER_SIZE = 655360;
	char buffer[CHUNK_BUFFER_SIZE];

	ssize_t bytesRead = recv(clientFd, buffer, CHUNK_BUFFER_SIZE, MSG_DONTWAIT);
	if (bytesRead > 0) {
		client.requestBuffer.append(buffer, bytesRead);
		client.lastActivity = time(NULL);

		// Check for request completion
		size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
		if (headerEnd != std::string::npos) {
			HTTPRequest tempRequest;
			if (tempRequest.parseHeaders(client.requestBuffer.substr(0, headerEnd))) {
				std::string contentLength = tempRequest.getHeader("Content-Length");
				bool isChunked = (tempRequest.getHeader("Transfer-Encoding") == "chunked");

				// Request is complete if:
				// 1. No body expected (no Content-Length and not chunked)
				// 2. Has Content-Length and we have all the data
				// 3. Chunked and we have the terminating chunk
				if (!isChunked && contentLength.empty()) {
					processCompleteRequests(clientFd, client);
					return;
				}

				if (!contentLength.empty()) {
					size_t expectedLength = std::atoi(contentLength.c_str());
					if (client.requestBuffer.length() >= headerEnd + 4 + expectedLength) {
						processCompleteRequests(clientFd, client);
						return;
					}
				}

				if (isChunked && client.requestBuffer.find("\r\n0\r\n\r\n") != std::string::npos) {
					processCompleteRequests(clientFd, client);
					return;
				}
			}
		}
	} else if (bytesRead == 0) {
		closeConnection(clientFd);
	} else if (errno != EAGAIN && errno != EWOULDBLOCK) {
		_logger.error("Read error: " + std::string(strerror(errno)));
		closeConnection(clientFd);
	}
}

void Server::processCompleteRequests(int clientFd, ClientState &client) {
	HTTPRequest request;

	try {
		if (!client.tempFile.empty()) {
			// Use file descriptor directly
			int fd = open(client.tempFile.c_str(), O_RDONLY);
			if (fd != -1) {
				request.setTempFilePath(client.tempFile);  // Let request handle the file
				client.tempFile.clear();  // Transfer ownership
			}
		} else {
			if (!request.parse(client.requestBuffer)) {
				sendBadRequestResponse(clientFd);
				return;
			}
		}
		std::string connection = request.getHeader("Connection");
		client.keepAlive = (connection == "keep-alive");

		std::string().swap(client.requestBuffer);

		RequestHandler handler(_config);
		client.response = handler.handleRequest(request);
		client.response.addHeader("Connection",
								  client.keepAlive ? "keep-alive" : "close");
		client.state = WRITING_RESPONSE;
		client.bytesWritten = 0;
		client.lastActivity = time(NULL);
	} catch (const std::exception& e) {
		_logger.error("Error processing request: " + std::string(e.what()));
		closeConnection(clientFd);
	}
}

void Server::handleNewConnection() {
	struct sockaddr_in addr = {};
	socklen_t addrLen = sizeof(addr);
	if (_clients.size() >= MAX_CLIENTS) {
		if (_clients.size() >= MAX_CLIENTS * 0.9) {  // 90% capacity
			checkIdleConnections();  // Force cleanup of idle connections
		}
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

	if (client.state != WRITING_RESPONSE) {
		return;
	}

	try {
		if (client.response.isFileDescriptor()) {
			bool continueStreaming = client.response.writeNextChunk(clientFd);
			if (!continueStreaming) {
				client.clear();
				client.state = IDLE;
				if (!client.keepAlive)
					closeConnection(clientFd);
			}
		} else {
			if (client.responseBuffer.empty()) {
				client.responseBuffer = client.response.toString();
			}

			ssize_t sent = send(clientFd,
								client.responseBuffer.c_str() + client.bytesWritten,
								client.responseBuffer.length() - client.bytesWritten,
								MSG_NOSIGNAL);

			if (sent > 0) {
				client.bytesWritten += sent;
				if (client.bytesWritten >= client.responseBuffer.length()) {
					client.clear();
					client.state = IDLE;
					if (!client.keepAlive)
						closeConnection(clientFd);
				}
			} else if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
				closeConnection(clientFd);
			}
		}
		client.lastActivity = time(NULL);
	} catch (const std::exception& e) {
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

	if (_clients.find(clientFd) != _clients.end()) {
		_clients[clientFd].clear();
		_clients.erase(clientFd);
	}

	try {
		shutdown(clientFd, SHUT_RDWR);
	} catch (...) {}

	close(clientFd);
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
	// Create upload directory with proper permissions
	std::string uploadPath = "www/upload";
	struct stat st;
	if (stat(uploadPath.c_str(), &st) != 0) {
		std::string mkdirCmd = "mkdir -p " + uploadPath;
		system(mkdirCmd.c_str());
		chmod(uploadPath.c_str(), 0755);
	}

	if (!initializeSocket()) {
		_logger.error("Failed to initialize socket: " + std::string(strerror(errno)));
		throw std::runtime_error("Failed to initialize socket");
	}

	_logger.info("Server initialized on " + _host + ":" + Utils::numToString(_port));
	updateMaxFileDescriptor();
}

void Server::handleExistingConnections(fd_set &readSet, fd_set &writeSet) {
	// First check and cleanup idle connections
	time_t currentTime = time(NULL);
	std::vector<int> toClose;

	for (std::map<int, ClientState>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		if (currentTime - it->second.lastActivity > IDLE_TIMEOUT ||
			(it->second.state == WRITING_RESPONSE &&
			 currentTime - it->second.lastActivity > KEEP_ALIVE_TIMEOUT)) {
			toClose.push_back(it->first);
		}
	}

	// Close idle connections
	for (size_t i = 0; i < toClose.size(); ++i) {
		closeConnection(toClose[i]);
	}

	// Handle active connections
	std::vector<int> activeClients;
	for (std::map<int, ClientState>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		activeClients.push_back(it->first);
	}

	for (size_t i = 0; i < activeClients.size(); ++i) {
		int fd = activeClients[i];
		if (_clients.find(fd) != _clients.end()) {  // Check if client still exists
			if (FD_ISSET(fd, &readSet)) {
				handleClientData(fd);
			}
			if (_clients.find(fd) != _clients.end() && FD_ISSET(fd, &writeSet)) {
				handleClientWrite(fd);
			}
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
