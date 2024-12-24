/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/26 00:01:46 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "../WebServ.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/Response.hpp"

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
			WRITING_RESPONSE
		};
		struct ClientState {
			ConnectionState state;
			std::string requestBuffer;
			std::string responseBuffer;
			time_t lastActivity;
			size_t contentLength;
			bool keepAlive;
			Response response;
			size_t bytesWritten;
			std::string tempFile;

			ClientState() :
					state(IDLE),
					lastActivity(time(NULL)),
					contentLength(0),
					keepAlive(true),
					response(200),
					bytesWritten(0) {}
			void clear() {
				state = IDLE;
				std::string().swap(requestBuffer);
				std::string().swap(responseBuffer);
				if (!tempFile.empty()) {
					unlink(tempFile.c_str());
					tempFile.clear();
				}
				contentLength = 0;
				bytesWritten = 0;
			}
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
		void updateMaxFileDescriptor();

		// Request processing
		void processCompleteRequests(int clientFd, ClientState &client);
		void sendBadRequestResponse(int clientFd);
};

#endif
