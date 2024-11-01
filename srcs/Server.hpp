/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:27:08 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 19:14:03 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "ConfigParser.hpp"
#include "RequestHandler.hpp"
#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <poll.h>

class Server {
	private:
		struct ListenSocket {
			int socketFd;
			std::string host;
			int port;
			ServerConfig* serverConfig;
		};

		std::vector<ListenSocket> _listenSockets;
		std::vector<ServerConfig> _serverConfigs;
		std::map<int, ServerConfig*> _clientToServerConfig;
		std::map<int, std::string> _clientBuffers;
		std::vector<struct pollfd> _pollFds;
		RequestHandler _requestHandler;

		// Setup methods
		void setupListeningSockets();
		void addToPollFds(int fd, short events);
		void removeFromPollFds(int fd);

		// Connection handling
		void acceptNewConnection(int listenFd);
		void handleClientData(int clientFd);
		void closeConnection(int clientFd);

		// Helper methods
		static bool setNonBlocking(int fd);
		static void setSocketOptions(int socketFd);

	public:
		explicit Server(const std::string& configFile);
		~Server();

		void run();
};

#endif // SERVER_HPP