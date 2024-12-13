/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/13 18:49:23 by sehosaf          ###   ########.fr       */
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
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(_serverSocket);
		return false;
	}
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
		close(_serverSocket);
		return false;
	}

	setNonBlocking(_serverSocket);

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(_serverSocket, (sockaddr *) &addr, sizeof(addr)) < 0) {
		close(_serverSocket);
		return false;
	}
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
	char buffer[BUFFER_SIZE] = {};
	ClientState &client = _clients[clientFd];
	client.lastActivity = time(NULL);

	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0)
		return (closeConnection(clientFd));
	client.requestBuffer.append(buffer, bytesRead);
	processCompleteRequests(clientFd, client);
}

bool Server::validateMethod(const std::string& method, const LocationConfig* location, ClientState& client) {
	if (!location) return true;

	for (std::vector<std::string>::const_iterator it = location->methods.begin(); it != location->methods.end(); ++it) {
		if (*it == method) return true;
	}

	Response response(405);
	response.addHeader("Content-Type", "text/html");
	std::string allowed;
	for (std::vector<std::string>::const_iterator it = location->methods.begin(); it != location->methods.end(); ++it) {
		if (!allowed.empty()) allowed += ", ";
		allowed += *it;
	}
	response.addHeader("Allow", allowed);
	response.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
	client.responseBuffer = response.toString();
	client.requestBuffer.clear();
	return false;
}

bool Server::validateContentLength(const std::string& requestBuffer, size_t headerEnd,
								   const LocationConfig* location, ClientState& client) {
	size_t clPos = requestBuffer.find("Content-Length: ");
	if (clPos == std::string::npos || clPos > headerEnd) {
		Response response(411, "Length Required");
		client.responseBuffer = response.toString();
		client.requestBuffer.clear();
		return false;
	}

	size_t clEnd = requestBuffer.find("\r\n", clPos);
	if (clEnd == std::string::npos) {
		Response response(400, "Bad Request");
		client.responseBuffer = response.toString();
		client.requestBuffer.clear();
		return false;
	}

	size_t contentLength = std::atol(requestBuffer.substr(clPos + 16, clEnd - (clPos + 16)).c_str());
	if (location && contentLength > location->client_max_body_size) {
		Response response = Response::makeErrorResponse(413);
		client.responseBuffer = response.toString();
		client.requestBuffer.clear();
		return false;
	}

	return true;
}

void Server::handlePostRequest(int clientFd, ClientState &client, size_t headerEnd) {
	const LocationConfig* location = _config.getLocation(
			client.requestBuffer.substr(
					client.requestBuffer.find(" ") + 1,
					client.requestBuffer.find(" HTTP/") - client.requestBuffer.find(" ") - 1
			)
	);
	// Validate content length header and size limits
	if (!validateContentLength(client.requestBuffer, headerEnd, location, client))
		return;

	// Get content length
	size_t clPos = client.requestBuffer.find("Content-Length: ");
	size_t clEnd = client.requestBuffer.find("\r\n", clPos);
	size_t contentLength = std::atol(
			client.requestBuffer.substr(clPos + 16, clEnd - (clPos + 16)).c_str()
	);
	// Check if received all the data
	if (client.requestBuffer.length() >= headerEnd + 4 + contentLength) {
		processRequest(clientFd, client);
		client.requestBuffer.clear();
	}
	// If not complete, wait for more data
}

void Server::processCompleteRequests(int clientFd, ClientState &client) {
	size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return;

	std::string transferEncoding;
	size_t tePos = client.requestBuffer.find("Transfer-Encoding: ");
	if (tePos != std::string::npos && tePos < headerEnd) {
		size_t teEnd = client.requestBuffer.find("\r\n", tePos);
		if (teEnd != std::string::npos)
			transferEncoding = client.requestBuffer.substr(tePos + 19, teEnd - (tePos + 19));
	}

	if (transferEncoding == "chunked") {
		// Look for the final chunk (0\r\n\r\n)
		if (client.requestBuffer.find("0\r\n\r\n", headerEnd) == std::string::npos)
			return; // Not all chunks received yet
	}

	size_t firstSpace = client.requestBuffer.find(" ");
	if (firstSpace == std::string::npos) {
		sendBadRequestResponse(clientFd);
		client.requestBuffer.clear();
		return;
	}

	std::string method = client.requestBuffer.substr(0, firstSpace);
	size_t secondSpace = client.requestBuffer.find(" ", firstSpace + 1);
	if (secondSpace == std::string::npos) {
		sendBadRequestResponse(clientFd);
		client.requestBuffer.clear();
		return;
	}

	const LocationConfig* location = _config.getLocation(
			client.requestBuffer.substr(
					client.requestBuffer.find(" ") + 1,
					client.requestBuffer.find(" HTTP/") - client.requestBuffer.find(" ") - 1
			)
	);

	if (!validateMethod(method, location, client))
		return;

	if (method == "POST") {
		handlePostRequest(clientFd, client, headerEnd);
	} else if (method == "GET" || method == "DELETE" || method == "PUT") {
		processRequest(clientFd, client);
		client.requestBuffer.clear();
	} else {
		Response response(501);
		response.addHeader("Content-Type", "text/html");
		response.setBody("<html><body><h1>501 Not Implemented</h1></body></html>");
		client.responseBuffer = response.toString();
		client.requestBuffer.clear();
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
	for (std::map<int, ClientState>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		close(it->first);
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

	for (std::map<int, ClientState>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (isConnectionIdle(currentTime, it->second))
			idleConnections.push_back(it->first);
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
	for (std::map<int, ClientState>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (it->first >= 0 && it->first < FD_SETSIZE)
			_maxFd = std::max(_maxFd, it->first);
}
