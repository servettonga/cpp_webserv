/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/06 18:55:44 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>

Server::Server(int port, std::string host) {
	/*
		Constructor implementation:
		1. Initialize server socket as -1
		2. Set the running flag to false
		3. Store port and host
		4. Initialize file descriptor sets
		5. Initialize maxFd to 0
	*/
	_maxFd = 0;
	_clients = std::map<int, ClientState>();
	_host = host;
	_port = port;
	_isRunning = false;
	_masterSet = fd_set();
	_readSet = fd_set();
	_writeSet = fd_set();
	_serverSocket = -1;
}

Server::~Server() {

}

void Server::start() {
	/*
		start():
		1. Initialize socket
		2. Set the running flag to true
		3. WHILE running:
		   - Copy master sets to working sets
		   - Set select timeout
		   - Call select() with all sets
		   - IF error:
			 Handle error (except EINTR)
		   - FOR each file descriptor:
			 IF readable:
			   IF server socket:
				 Handle new connection
			   ELSE:
				 Handle client data
			 IF writable:
			   IF client has pending response:
				 Handle client write
	*/
	std::cout << "Server started on " << _host << ":" << _port << std::endl;
}

void Server::stop() {
	/*
		stop():
		1. Set running flag to false
		2. Close all client connections
		3. Clear clients map
		4. Close server socket
		5. Clear file descriptor sets
	*/
}

// ********** Socket setup methods **********

void Server::initializeSocket() {
	/*
		initializeSocket():
		1. Create socket
		2. Set socket options (reuse address)
		3. Set non-blocking mode
		4. Bind to specified port and address
		5. Listen for connections
		6. Add server socket to master set
		7. Update maxFd
	*/
}

// Set socket to non-blocking mode
void Server::setNonBlocking(int sockfd) {
	(void)sockfd;
}

// ********** Connection handling methods **********

void Server::handleNewConnection() {
	/*
		handleNewConnection():
		1. Accept new connection
		2. IF error:
		   Handle error (except would block)
		3. Set new socket non-blocking
		4. Add to master set
		5. Update maxFd if needed
		6. Initialize client state
		7. Add to the clients map
	*/
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
	(void)clientFd;
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
	(void)clientFd;
}

// Clean up client connection
void Server::closeConnection(int clientFd) {
	(void)clientFd;
}

// ********** Request/Response methods **********

bool Server::processRequest(int clientFd, Server::ClientState &client) {
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
	(void)clientFd;
	(void)client;
	return false;
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
		4. Create simple HTML error page with message
		5. Set response body
		6. Convert response to string
		7. Set client's response buffer
	*/
	(void)clientFd;
	(void)statusCode;
	(void)message;
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
	(void)path;
	(void)headers;
	(void)response;
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
	(void)path;
	(void)headers;
	(void)body;
	(void)response;
}

// ********** Helper methods **********

// Update the highest file descriptor
void Server::updateMaxFd() {

}

// Check if full request received
bool Server::isRequestComplete(const std::string &request) {
	(void)request;
	return false;
}
