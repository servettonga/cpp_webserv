/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/05 12:03:39 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "ServerConfig.hpp"
#include "../utils/Logger.hpp"
#include <map>
#include <string>
#include <sys/select.h>

#define BUFFER_SIZE 8192
#define KEEP_ALIVE_TIMEOUT 60    // 60 seconds for keep-alive connections
#define IDLE_TIMEOUT 120         // 120 seconds for idle connections

class Server {
	public:
		explicit Server(const ServerConfig &config);
		~Server();

		void initialize();
		void stop();

		void handleNewConnection();
		void handleExistingConnections(fd_set &readSet, fd_set &writeSet);
		int getServerSocket() const { return _serverSocket; }
		int getMaxFd() const { return _maxFd; }

		enum ConnectionState {
			IDLE,
			READING_REQUEST,
			WRITING_RESPONSE
		};
		struct ClientState {
			ConnectionState state;
			std::string requestBuffer;
			std::string responseBuffer;
			time_t lastActivity;

			ClientState() : state(IDLE), lastActivity(time(NULL)) {}
		};
		const std::map<int, ClientState> &getClients() const { return _clients; }

	private:
		static const int MAX_CLIENTS = FD_SETSIZE - 10;  // Reserve some FDs for system use

		// Server configuration
		const std::string _host;
		const int _port;
		int _serverSocket;
		ServerConfig _config;
		static Logger &_logger;

		// Socket management
		int _maxFd;
		std::map<int, ClientState> _clients;

		void checkIdleConnections();
		bool isConnectionIdle(time_t currentTime, const ClientState &client) const;

		// Socket initialization
		bool initializeSocket();
		void setNonBlocking(int sockfd);

		// Client handling
		void handleClientData(int clientFd);
		void handleClientWrite(int clientFd);
		void closeConnection(int clientFd);
		bool shouldCloseConnection(const ClientState &client) const;
		void updateMaxFileDescriptor();

		// Request processing
		void processCompleteRequests(int clientFd, ClientState &client);
		void sendBadRequestResponse(int clientFd);
};

#endif
