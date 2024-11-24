/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/24 12:43:14 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "../utils/Utils.hpp"
#include <cstring>		// For strerror
#include <iostream>		// For std::cout
#include <sys/socket.h>	// For socket functions
#include <netinet/in.h>	// For sockaddr_in
#include <cerrno>		// For errno
#include <fcntl.h>		// For fcntl
#include <sstream>		// For stringstream
#include <fstream>		// For file I/O
#include <dirent.h>     // For opendir, readdir, closedir, DIR
#include <sys/types.h>  // For basic system data types
#include <sys/stat.h>   // For stat() function
#include <unistd.h>     // For basic system calls
#include <time.h>       // For time functions like strftime

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
	ClientState client = {"", "", false}; // Initialize client state
	_clients[clientFd] = client;
}

void Server::handleClientData(int clientFd) {
	char buffer[1024] = {};
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0) {
		if (bytesRead < 0 && errno != EWOULDBLOCK && errno != EAGAIN)
			throw std::runtime_error("Failed to receive: " + std::string(strerror(errno)));
		closeConnection(clientFd);
		return;
	}
	ClientState &client = _clients[clientFd];
	client.requestBuffer.append(buffer, bytesRead);
	if (!client.requestComplete && isRequestComplete(client.requestBuffer)) {
		processRequest(clientFd, client);
		client.requestComplete = true;
	}
}

void Server::handleClientWrite(int clientFd) {
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
	// Size limit check
	if (client.requestBuffer.size() > 8192)
		return (sendErrorResponse(clientFd, 413, "Payload Too Large"));
	// Parse request
	HTTPRequest request;
	if (!request.parse(client.requestBuffer))
		return (sendErrorResponse(clientFd, 400, "Bad Request"));
	// Create response
	Response response(200);
	// Handle connection persistence
	if (request.getVersion() == "HTTP/1.1") {
		std::string connection = request.getHeader("Connection");
		response.addHeader("Connection",
						   (connection == "close") ? "close" : "keep-alive");
	} else {
		std::string connection = request.getHeader("Connection");
		response.addHeader("Connection",
						   (connection == "keep-alive") ? "keep-alive" : "close");
	}
	// Route request
	if (request.getMethod() == "GET") {
		handleGET(request.getPath(), response);
	} else if (request.getMethod() == "POST") {
		handlePOST(request.getPath(), request.getHeaders(),
				   request.getBody(), response);
	} else {
		response.setStatusCode(405);
		response.addHeader("Allow", "GET, POST");
		response.setBody("Method Not Allowed");
	}
	client.responseBuffer = response.toString();
}

void Server::sendErrorResponse(int clientFd, int statusCode, const std::string &message) {
	Response response(statusCode);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>" + message + "</h1></body></html>");
	std::string responseStr = response.toString();
	_clients[clientFd].responseBuffer = responseStr;
}

// ********** HTTP method handlers **********

void Server::handleGET(const std::string &path, Response &response) {
	// Security check
	if (path.find("..") != std::string::npos || path.find('~') != std::string::npos) {
		response.setStatusCode(403);
		response.setBody("Forbidden");
		return;
	}
	// Handle /files route
	if (path.substr(0, 6) == "/files") {
		std::string filesPath = "www/files" + (path.length() > 6 ? path.substr(6) : "");
		struct stat statbuf;
		if (stat(filesPath.c_str(), &statbuf) == 0) {
			if (S_ISDIR(statbuf.st_mode)) {
				// Directory listing
				std::string listing = createDirectoryListing(filesPath, path);
				if (listing.empty()) {
					if (errno == EACCES)
						response.setStatusCode(403);
					else if (errno == ENOENT)
						response.setStatusCode(404);
					else
						response.setStatusCode(500);
					response.setBody("Directory access error");
				} else {
					response.setStatusCode(200);
					response.addHeader("Content-Type", "text/html");
					response.setBody(listing);
				}
				return;
			}
		}
	}
	// Regular file handling
	std::string fullPath;
	if (path == "/" || path == "/index.html")
		fullPath = "www/index.html";
	else
		fullPath = "www" + path;
	// File serving
	std::ifstream file(fullPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		response.setStatusCode(404);
		response.setBody("Not Found");
		return;
	}
	// Read file content
	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	// Set response
	response.setStatusCode(200);
	std::string ext = fullPath.substr(fullPath.find_last_of('.') + 1);
	std::string mimeType = HTTPUtils::getMimeType(ext);
	// Set Content-Type for viewable files
	response.addHeader("Content-Type", mimeType);
	// Add Content-Disposition for non-viewable files
	if (mimeType == "application/octet-stream" ||
		mimeType == "application/zip" ||
		mimeType == "application/x-executable") {
		response.addHeader("Content-Disposition", "attachment; filename=\"" +
												  path.substr(path.find_last_of('/') + 1) + "\"");
	}
	response.setBody(buffer.str());
}

void Server::handlePOST(const std::string &path,
						const std::map<std::string, std::string> &headers,
						const std::string &body,
						Response &response) {
	// Validate headers
	std::map<std::string, std::string>::const_iterator contentType = headers.find("Content-Type");
	std::map<std::string, std::string>::const_iterator contentLength = headers.find("Content-Length");

	if (contentType == headers.end()) {
		response.setStatusCode(400);
		response.setBody("Bad Request: Missing Content-Type");
		return;
	}
	if (contentLength == headers.end()) {
		response.setStatusCode(411);
		response.setBody("Length Required");
		return;
	}
	// Check body size
	if (body.length() > 8192) { // 8KB limit
		response.setStatusCode(413);
		response.setBody("Payload Too Large");
		return;
	}
	// Basic POST handling - echo back
	response.setStatusCode(200);
	response.addHeader("Content-Type", "text/plain");
	response.setBody("Received POST to " + path + "\nBody: " + body);
}

// Check if full request received
bool Server::isRequestComplete(const std::string &request) {
	return (request.find("\r\n\r\n") != std::string::npos);
}

std::string Server::createDirectoryListing(const std::string &path, const std::string &urlPath) {
	DIR* dir = opendir(path.c_str());
	if (!dir)
		return "";
	std::stringstream html;
	html << "<html><head><title>Directory: " << urlPath << "</title></head><body>\n"
		 << "<h1>Directory: " << urlPath << "</h1>\n"
		 << "<table>\n"
		 << "<tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n";
	// Add the parent directory link only if not in root
	if (urlPath != "/files/") {
		html << "<tr><td><a href=\"../\">..</a></td><td>-</td><td>-</td></tr>\n";
	}
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		struct stat statbuf;
		std::string fullPath = path + "/" + name;
		if (stat(fullPath.c_str(), &statbuf) == 0) {
			// Remove trailing slash from urlPath if exists
			std::string cleanUrlPath = urlPath;
			if (cleanUrlPath[cleanUrlPath.length() - 1] == '/')
				cleanUrlPath = cleanUrlPath.substr(0, cleanUrlPath.length() - 1);
			html << "<tr><td><a href=\"";
			if (urlPath == "/files/")
				html << "/files/" << name;
			else
				html << cleanUrlPath << "/" << name;
			// Add trailing slash only for directories
			if (S_ISDIR(statbuf.st_mode))
				html << "/";
			html << "\">" << name << "</a></td>";
			if (S_ISDIR(statbuf.st_mode))
				html << "<td>-</td>";
			else
				html << "<td>" << statbuf.st_size << " bytes</td>";
			char timebuf[32];
			strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
					 localtime(&statbuf.st_mtime));
			html << "<td>" << timebuf << "</td></tr>\n";
		}
	}
	closedir(dir);
	html << "</table></body></html>";
	return html.str();
}
