/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerGroup.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/04 11:30:42 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/04 19:55:16 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerGroup.hpp"
#include <algorithm>
#include <iostream>
#include <sys/select.h>
#include <cerrno>
#include <cstring>

ServerGroup::ServerGroup() : _isRunning(false), _maxFd(0) {
	FD_ZERO(&_masterSet);
	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
}

ServerGroup::~ServerGroup() {
	stop();
}

void ServerGroup::addServer(const ServerConfig& config) {
	Server* server = new Server(config);
	_servers.push_back(server);
}

void ServerGroup::handleEvents(fd_set& readSet, fd_set& writeSet) {
	for (std::vector<Server*>::iterator it = _servers.begin();	// Handle all server events first
		 it != _servers.end(); ++it) {
		Server* server = *it;
		if (FD_ISSET(server->getServerSocket(), &readSet))	// Handle new connections first
			server->handleNewConnection();
	}

	// Then handle existing connections
	for (std::vector<Server*>::iterator it = _servers.begin();
		 it != _servers.end(); ++it) {
		(*it)->handleExistingConnections(readSet, writeSet);
	}

	// Rebuild fd sets
	FD_ZERO(&_masterSet);
	FD_ZERO(&_writeSet);

	for (std::vector<Server*>::iterator it = _servers.begin();
		 it != _servers.end(); ++it) {
		Server* server = *it;
		int serverFd = server->getServerSocket();
		// Add server socket if valid
		if (serverFd >= 0)
			FD_SET(serverFd, &_masterSet);
		// Add client sockets
		const std::map<int, Server::ClientState>& clients = server->getClients();
		for (std::map<int, Server::ClientState>::const_iterator cit = clients.begin();
			 cit != clients.end(); ++cit) {
			if (cit->first >= 0 && cit->first < FD_SETSIZE) {
				FD_SET(cit->first, &_masterSet);
				if (!cit->second.responseBuffer.empty())
					FD_SET(cit->first, &_writeSet);
			}
		}
	}

	updateMaxFd();
}

void ServerGroup::start() {
	if (_servers.empty())
		throw std::runtime_error("No servers configured");

	_isRunning = true;
	initializeServers();

	while (_isRunning && !_servers.empty()) {
		if (!handleSelect())
			continue;
	}
}

void ServerGroup::stop() {
	_isRunning = false;
	for (std::vector<Server*>::iterator it = _servers.begin();
		 it != _servers.end(); ++it) {
		(*it)->stop();
		delete *it;
	}
	_servers.clear();
	FD_ZERO(&_masterSet);
	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
}

void ServerGroup::updateMaxFd() {
	_maxFd = 0;
	for (std::vector<Server*>::iterator it = _servers.begin();
		 it != _servers.end(); ++it) {
		_maxFd = std::max(_maxFd, (*it)->getMaxFd());
	}
}

bool ServerGroup::handleSelect() {
	fd_set readSet = _masterSet;
	fd_set writeSet = _writeSet;

	timeval timeout = {SELECT_TIMEOUT_SEC, SELECT_TIMEOUT_USEC};
	int activity = select(_maxFd + 1, &readSet, &writeSet, NULL, &timeout);

	if (activity < 0) {
		if (errno == EINTR)
			return false;
		throw std::runtime_error("Select failed: " + std::string(strerror(errno)));
	}
	if (activity > 0)
		handleEvents(readSet, writeSet);
	return true;
}

void ServerGroup::initializeServers() {
	FD_ZERO(&_masterSet);
	FD_ZERO(&_writeSet);

	for (std::vector<Server*>::iterator it = _servers.begin();
		 it != _servers.end(); ++it) {
		try {
			(*it)->initialize();
			FD_SET((*it)->getServerSocket(), &_masterSet);
			updateMaxFd();
		} catch (const std::exception& e) {
			cleanupServers();
			throw;
		}
	}
}

void ServerGroup::cleanupServers() {
	stop();
}
