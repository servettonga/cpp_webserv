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
#include <map>
#include <string>
#include <sys/select.h>

#define BUFFER_SIZE (128 * 1024)	// 128KB

class Server {
	public:
		explicit Server(const ServerConfig &config);
		~Server();

		void initialize();
		void stop();

		struct ClientState {
			std::string requestBuffer;
			std::string responseBuffer;
			std::vector<char> writeBuffer;
			size_t writeBufferSize;
			time_t lastActivity;

			ClientState();
		};

		void handleNewConnection();
		void handleExistingConnections(fd_set &readSet, fd_set &writeSet);
		int getServerSocket() const { return _serverSocket; }
		int getMaxFd() const { return _maxFd; }
		const std::map<int, ClientState> &getClients() const { return _clients; }

	private:
		static const size_t DEFAULT_WRITE_BUFFER_SIZE = 1024 * 1024;  // 1MB

		// Server configuration
		const std::string _host;
		const int _port;
		int _serverSocket;
		const ServerConfig _config;
		static const int IDLE_TIMEOUT = 300;	// 5 minutes in seconds

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
		void processRequest(int clientFd, ClientState &client);
		void sendBadRequestResponse(int clientFd);
		void handlePostRequest(int clientFd, ClientState &client, size_t headerEnd);
		bool validateMethod(const std::string &method, const LocationConfig *location, ClientState &client);
		bool validateContentLength(const std::string &requestBuffer, size_t headerEnd, const LocationConfig *location,
								   ClientState &client);
};

#endif
