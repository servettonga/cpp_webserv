/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 23:05:58 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "ServerConfig.hpp"
#include <map>
#include <string>
#include <sys/select.h>

#define BUFFER_SIZE (128 * 1024)	// 128KB

class Server {
	private:
		struct ClientState {
			std::string requestBuffer;
			std::string responseBuffer;
			std::vector<char> writeBuffer;
			size_t writeBufferSize;

			ClientState();
		};

		// Server configuration
		const std::string _host;
		const int _port;
		int _serverSocket;
		bool _isRunning;
		const ServerConfig _config;
		static const int IDLE_TIMEOUT = 300;	// 5 minutes in seconds
		time_t _lastActivity;         // Track last activity timestamp

		// Socket management
		fd_set _masterSet;
		fd_set _readSet;
		fd_set _writeSet;
		int _maxFd;
		std::map<int, ClientState> _clients;

		// Socket initialization
		bool initializeSocket();
		void setNonBlocking(int sockfd);

		// Event loop
		void runEventLoop();
		void handleEvents(fd_set &readSet, fd_set &writeSet);
		void handleReadEvent(int fd);

		// Client handling
		void handleNewConnection();
		void handleClientData(int clientFd);
		void handleClientWrite(int clientFd);
		void closeConnection(int clientFd);
		bool shouldCloseConnection(const ClientState &client) const;

		// Request processing
		void processCompleteRequests(int clientFd, ClientState &client);
		void processRequest(int clientFd, ClientState &client);
		void sendBadRequestResponse(int clientFd);

	public:
		explicit Server(const ServerConfig &config);
		~Server();

		void start();
		void stop();

};

#endif
