/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:27:08 by sehosaf           #+#    #+#             */
/*   Updated: 2024/10/28 15:47:12 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>

Server::Server(const std::map<std::string, std::string> &config) : config(config), listen_fd(-1) {}

bool Server::initialize() {
	return setupSocket();
}

bool Server::setupSocket() {
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
		return false;
	}

	int opt = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		std::cerr << "Set socket options failed: " << strerror(errno) << std::endl;
		return false;
	}

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(atoi(config["port"].c_str()));

	if (bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
		std::cerr << "Bind failed: " << strerror(errno) << std::endl;
		return false;
	}

	if (listen(listen_fd, 10) < 0) {
		std::cerr << "Listen failed: " << strerror(errno) << std::endl;
		return false;
	}

	fcntl(listen_fd, F_SETFL, O_NONBLOCK);

	pollfd pfd = {};
	pfd.fd = listen_fd;
	pfd.events = POLLIN;
	poll_fds.push_back(pfd);

	std::cout << "Server is listening on port " << config["port"] << std::endl;
	return true;
}

void Server::run() {
	while (true) {
		const int ret = poll(&poll_fds[0], poll_fds.size(), -1);
		if (ret < 0) {
			std::cerr << "Poll error: " << strerror(errno) << std::endl;
			continue;
		}

		for (size_t i = 0; i < poll_fds.size(); ++i) {
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == listen_fd) {
					handleConnections();
				} else {
					handleRequest(poll_fds[i].fd);
				}
			}
		}
	}
}

void Server::handleConnections() {
	sockaddr_in client_addr = {};
	socklen_t          client_len = sizeof(client_addr);
	const int          client_fd = accept(listen_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
	if (client_fd < 0) return;

	fcntl(client_fd, F_SETFL, O_NONBLOCK);

	pollfd pfd = {};
	pfd.fd = client_fd;
	pfd.events = POLLIN;
	poll_fds.push_back(pfd);
}

void Server::handleRequest(const int client_fd) {
	char      buffer[1024];
	const int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
	if (bytes_read <= 0) {
		close(client_fd);
		return;
	}

	buffer[bytes_read] = '\0';
	std::string request(buffer);

	const std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
	write(client_fd, response.c_str(), response.size());
	close(client_fd);
}
