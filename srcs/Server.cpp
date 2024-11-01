/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 22:54:24 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <algorithm>

/*
	Server class implementation:
	- The Server class initializes by parsing the configuration file
 	and setting up listening sockets based on the configurations provided.
	- The run() method enters the main loop, using poll() to monitor sockets for incoming connections and data.
	- When a new connection is accepted, it's added to the list of client connections,
 	and the corresponding server configuration is associated with it.
	- The handleClientData() method reads incoming data from client sockets and checks for complete HTTP requests.
	- Once a complete request is received, it delegates the handling to the RequestHandler class.
	- Error handling is included to manage socket errors and client disconnections gracefully.
 */

Server::Server(const std::string& configFile)
		: _serverConfigs(ConfigParser(configFile).getServerConfigs()),
		  _requestHandler(_serverConfigs)
{
	if (_serverConfigs.empty()) {
		throw std::runtime_error("No server configurations found");
	}
	setupListeningSockets();
}

Server::~Server() {
	// Clean up all open sockets
	std::vector<ListenSocket>::iterator it;
	for (it = _listenSockets.begin(); it != _listenSockets.end(); ++it) {
		if (it->socketFd >= 0) {
			close(it->socketFd);
		}
	}

	// Clean up client connections
	std::map<int, std::string>::iterator clientIt;
	for (clientIt = _clientBuffers.begin(); clientIt != _clientBuffers.end(); ++clientIt) {
		if (clientIt->first >= 0) {
			close(clientIt->first);
		}
	}
}

void Server::setupListeningSockets() {
	std::vector<ServerConfig>::iterator it;
	for (it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
		int socketFd = socket(AF_INET, SOCK_STREAM, 0);
		if (socketFd < 0) {
			throw std::runtime_error("Failed to create socket");
		}

		setSocketOptions(socketFd);
		if (!setNonBlocking(socketFd)) {
			close(socketFd);
			throw std::runtime_error("Failed to set socket non-blocking");
		}

		sockaddr_in addr = {};
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(it->port);
		addr.sin_addr.s_addr = inet_addr(it->host.c_str());

		if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			close(socketFd);
			throw std::runtime_error("Failed to bind socket");
		}

		if (listen(socketFd, SOMAXCONN) < 0) {
			close(socketFd);
			throw std::runtime_error("Failed to listen on socket");
		}

		ListenSocket listenSocket;
		listenSocket.socketFd = socketFd;
		listenSocket.host = it->host;
		listenSocket.port = it->port;
		listenSocket.serverConfig = &(*it);
		_listenSockets.push_back(listenSocket);

		addToPollFds(socketFd, POLLIN);
	}
}

void Server::run() {
	while (true) {
		int pollResult = poll(&_pollFds[0], _pollFds.size(), -1);
		if (pollResult < 0) {
			if (errno == EINTR) {
				continue;
			}
			throw std::runtime_error("Poll failed");
		}

		std::vector<struct pollfd>::iterator it;
		for (it = _pollFds.begin(); it != _pollFds.end(); ++it) {
			if (it->revents == 0) {
				continue;
			}

			if (it->revents & POLLIN) {
				// Check if it's a listening socket
				bool isListening = false;
				std::vector<ListenSocket>::iterator listenIt;
				for (listenIt = _listenSockets.begin(); listenIt != _listenSockets.end(); ++listenIt) {
					if (it->fd == listenIt->socketFd) {
						acceptNewConnection(it->fd);
						isListening = true;
						break;
					}
				}

				if (!isListening) {
					handleClientData(it->fd);
				}
			}

			if (it->revents & (POLLHUP | POLLERR | POLLNVAL)) {
				closeConnection(it->fd);
			}
		}
	}
}

void Server::acceptNewConnection(int listenFd) {
	sockaddr_in clientAddr = {};
	socklen_t clientLen = sizeof(clientAddr);

	int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientLen);
	if (clientFd < 0) {
		return;
	}

	if (!setNonBlocking(clientFd)) {
		close(clientFd);
		return;
	}

	// Find the corresponding server config
	std::vector<ListenSocket>::iterator it;
	for (it = _listenSockets.begin(); it != _listenSockets.end(); ++it) {
		if (it->socketFd == listenFd) {
			_clientToServerConfig[clientFd] = it->serverConfig;
			break;
		}
	}

	addToPollFds(clientFd, POLLIN);
	_clientBuffers[clientFd] = "";
}

void Server::handleClientData(int clientFd) {
	char buffer[4096];
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);

	if (bytesRead <= 0) {
		closeConnection(clientFd);
		return;
	}

	_clientBuffers[clientFd].append(buffer, bytesRead);

	// Check if we have a complete HTTP request
	std::string& clientBuffer = _clientBuffers[clientFd];
	size_t pos = clientBuffer.find("\r\n\r\n");
	if (pos != std::string::npos) {
		// Handle the complete request
		_requestHandler.handleRequest(clientBuffer, clientFd);
		clientBuffer.clear();
	}
}

void Server::closeConnection(int clientFd) {
	close(clientFd);
	removeFromPollFds(clientFd);
	_clientToServerConfig.erase(clientFd);
	_clientBuffers.erase(clientFd);
}

void Server::addToPollFds(int fd, short events) {
	pollfd pfd = {};
	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	_pollFds.push_back(pfd);
}

void Server::removeFromPollFds(int fd) {
	std::vector<struct pollfd>::iterator it;
	for (it = _pollFds.begin(); it != _pollFds.end(); ++it) {
		if (it->fd == fd) {
			_pollFds.erase(it);
			break;
		}
	}
}

bool Server::setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) {
		return false;
	}
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

void Server::setSocketOptions(int socketFd) {
	int opt = 1;
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(socketFd);
		throw std::runtime_error("Failed to set socket options");
	}
}

/*
	TODO: Ensuring Non-Blocking I/O
	Make sure that all read and write operations are performed after checking socket readiness with poll().

	Important:
	- Do not perform any read or write operations on a socket unless poll() indicates that it is ready.
	- This adherence is crucial to meet the project requirements.
 */
