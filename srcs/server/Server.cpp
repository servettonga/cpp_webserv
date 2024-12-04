/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/04 15:53:10 by sehosaf          ###   ########.fr       */
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
		_config(config),
		_maxFd(0) {
}

Server::~Server() {
	stop();
	std::cout << "Server " << _host << " stopped" << std::endl;
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
	char buffer[BUFFER_SIZE] = {};
	ClientState &client = _clients[clientFd];
	client.lastActivity = time(NULL);

	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0)
		return (closeConnection(clientFd));
	client.requestBuffer.append(buffer, bytesRead);
	processCompleteRequests(clientFd, client);
}

void Server::processCompleteRequests(int clientFd, ClientState &client) {
	size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return;

	if (client.requestBuffer.find("POST") == 0) {
		size_t clPos = client.requestBuffer.find("Content-Length: ");
		if (clPos == std::string::npos || clPos > headerEnd) {
			Response response(411, "Length Required");
			client.responseBuffer = response.toString();
			client.requestBuffer.clear();
			return;
		}
		// Parse Content-Length value
		size_t clEnd = client.requestBuffer.find("\r\n", clPos);
		if (clEnd == std::string::npos) {
			Response response(400, "Bad Request");
			client.responseBuffer = response.toString();
			client.requestBuffer.clear();
			return;
		}
		size_t contentLength = std::atol(
				client.requestBuffer.substr(clPos + 16, clEnd - (clPos + 16)).c_str()
		);

		// Get location and check the size limit before accepting body
		const LocationConfig *location = _config.getLocation(
				client.requestBuffer.substr(
						client.requestBuffer.find(" ") + 1,
						client.requestBuffer.find(" HTTP/") - client.requestBuffer.find(" ") - 1
				)
		);
		if (location && contentLength > location->client_max_body_size) {
			Response response = Response::makeErrorResponse(413);
			client.responseBuffer = response.toString();
			client.requestBuffer.clear();
			return;
		}
		if (client.requestBuffer.length() >= headerEnd + 4 + contentLength) {
			processRequest(clientFd, client);
			client.requestBuffer.clear();
		}
		return;
	}

	if (client.requestBuffer.find("GET") == 0 ||
		client.requestBuffer.find("DELETE") == 0) {
		processRequest(clientFd, client);
		client.requestBuffer.clear();
		return;
	}
}

void Server::handleNewConnection() {
	sockaddr_in addr = {};
	socklen_t addrLen = sizeof(addr);

	int clientFd = accept(_serverSocket, (sockaddr *) &addr, &addrLen);
	if (clientFd < 0)
		return;

	setNonBlocking(clientFd);
	_clients[clientFd] = ClientState();
	updateMaxFileDescriptor();
}

void Server::handleClientWrite(int clientFd) {
	ClientState &client = _clients[clientFd];
	client.lastActivity = time(NULL);
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
}

void Server::sendBadRequestResponse(int clientFd) {
	Response response(400);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>Bad Request</h1></body></html>");
	_clients[clientFd].responseBuffer = response.toString();
}

void Server::closeConnection(int clientFd) {
	close(clientFd);
	_clients.erase(clientFd);
}

void Server::stop() {
	for (std::map<int, ClientState>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		close(it->first);
	}
	_clients.clear();
	close(_serverSocket);
}

void Server::initialize() {
	if (!initializeSocket())
		throw std::runtime_error("Failed to initialize socket");

	updateMaxFileDescriptor();
	std::cout << "Server " << _host << " started on port: " << _port << std::endl;
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

	for (std::map<int, ClientState>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		if (isConnectionIdle(currentTime, it->second))
			idleConnections.push_back(it->first);
	}
	for (size_t i = 0; i < idleConnections.size(); ++i)
		closeConnection(idleConnections[i]);
}

bool Server::isConnectionIdle(time_t currentTime, const ClientState &client) const {
	return (currentTime - client.lastActivity) > IDLE_TIMEOUT;
}

Server::ClientState::ClientState() : writeBufferSize(DEFAULT_WRITE_BUFFER_SIZE), lastActivity(time(NULL)) {
	writeBuffer.reserve(writeBufferSize);
}

void Server::updateMaxFileDescriptor() {
	_maxFd = _serverSocket;
	for (std::map<int, ClientState>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->first >= 0 && it->first < FD_SETSIZE)
			_maxFd = std::max(_maxFd, it->first);
	}
}
