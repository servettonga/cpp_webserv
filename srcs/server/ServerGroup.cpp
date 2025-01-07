/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerGroup.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/04 11:30:42 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:39:05 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerGroup.hpp"
#include "../config/ConfigParser.hpp"

ServerGroup *ServerGroup::_instance = NULL;
bool		 ServerGroup::_shutdownRequested = false;
std::string	 ServerGroup::_configFile;

ServerGroup::ServerGroup(const std::string &configFile) : _isRunning(false), _maxFd(0) {
	FD_ZERO(&_masterSet);
	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
	_configFile = configFile;
	_instance = this;
}

ServerGroup::~ServerGroup() {
	stop();
}

void ServerGroup::addServer(const ServerConfig &config) {
	ServerConfig serverConfig = config;
	serverConfig.precomputePaths();

	Server *server = new Server(serverConfig);
	_servers.push_back(server);
}

void ServerGroup::handleEvents(fd_set &readSet, fd_set &writeSet) {
	for (std::vector<Server *>::iterator it = _servers.begin(); // Handle all server events first
		 it != _servers.end(); ++it) {
		Server *server = *it;
		if (FD_ISSET(server->getServerSocket(), &readSet)) // Handle new connections first
			server->handleNewConnection();
	}

	// Then handle existing connections
	for (std::vector<Server *>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
		(*it)->handleExistingConnections(readSet, writeSet);
	}

	// Rebuild fd sets
	FD_ZERO(&_masterSet);
	FD_ZERO(&_writeSet);

	for (std::vector<Server *>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
		Server *server = *it;
		int		serverFd = server->getServerSocket();
		if (serverFd >= 0)
			FD_SET(serverFd, &_masterSet);

		// Add client sockets to write set if they have pending data
		const std::map<int, Server::ClientState> &clients = server->getClients();
		for (std::map<int, Server::ClientState>::const_iterator cit = clients.begin(); cit != clients.end(); ++cit) {
			if (cit->first >= 0 && cit->first < FD_SETSIZE) {
				FD_SET(cit->first, &_masterSet);
				if (cit->second.state == Server::WRITING_RESPONSE)
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
	_shutdownRequested = false;
	setupSignalHandlers();

	try {
		initializeServers();
		while (_isRunning && !_shutdownRequested && !_servers.empty())
			if (!handleSelect())
				continue;
	} catch (...) {
		cleanup();
		throw;
	}

	cleanup();
}

void ServerGroup::stop() {
	_isRunning = false;
	for (std::vector<Server *>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
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
	for (std::vector<Server *>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
		_maxFd = std::max(_maxFd, (*it)->getMaxFd());
	}
}

bool ServerGroup::handleSelect() {
	fd_set readSet = _masterSet;
	fd_set writeSet = _writeSet;

	timeval timeout = {SELECT_TIMEOUT_SEC, SELECT_TIMEOUT_USEC};
	int		activity = select(_maxFd + 1, &readSet, &writeSet, NULL, &timeout);

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

	for (std::vector<Server *>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
		try {
			(*it)->initialize();
			FD_SET((*it)->getServerSocket(), &_masterSet);
			updateMaxFd();
		} catch (const std::exception &e) {
			cleanup();
			throw;
		}
	}
}

void ServerGroup::setupSignalHandlers() {
	struct sigaction sa = {};
	sa.sa_handler = ServerGroup::signalHandler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1 ||
		sigaction(SIGQUIT, &sa, NULL) == -1 || sigaction(SIGHUP, &sa, NULL) == -1) {
		throw std::runtime_error("Failed to set up signal handlers");
	}
}

void ServerGroup::signalHandler(int signum) {
	if (signum == SIGHUP) {
		std::cout << YELLOW << "Received SIGHUP, reloading configuration...\n" << RESET;
		if (_instance)
			_instance->reloadConfiguration(_instance->_configFile);
	} else {
		std::cout << YELLOW << "\nReceived signal " << signum << ", shutting down...\n" << RESET;
		_shutdownRequested = true;
	}
}

void ServerGroup::cleanup() {
	stop();
}

void ServerGroup::reloadConfiguration(const std::string &configFile) {
	try {
		ConfigParser parser(configFile);
		parser.reload();
		if (!parser.validate()) {
			std::cerr << RED << "Configuration validation failed during reload:\n" << RESET;
			std::vector<std::string> errors = parser.getErrors();
			for (std::vector<std::string>::const_iterator it = errors.begin(); it != errors.end(); ++it) {
				std::cerr << *it << std::endl;
			}
			return;
		}
		std::vector<ServerConfig> newConfigs = parser.parse(); // Parse new configuration
		if (newConfigs.empty()) {
			std::cerr << YELLOW << "No valid server configurations found during reload\n" << RESET;
			return;
		}
		stop(); // Stop existing servers
		// Start new servers
		for (std::vector<ServerConfig>::iterator it = newConfigs.begin(); it != newConfigs.end(); ++it) {
			addServer(*it);
		}
		initializeServers();
		_isRunning = true;
		std::cout << "Configuration reloaded successfully\n";
	} catch (const std::exception &e) {
		std::cerr << RED << "Failed to reload configuration: " << e.what() << RESET << std::endl;
	}
}
