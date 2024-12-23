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
#include "../http/Response.hpp"
#include <map>
#include <string>
#include <sys/select.h>
#include <sstream>

#define BUFFER_SIZE 65536
#define KEEP_ALIVE_TIMEOUT 120
#define IDLE_TIMEOUT 60

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
			size_t contentLength;
			bool continueSent;
			bool keepAlive;
			bool isChunked;
			bool waitForEOF;
			Response response;
			bool isStreaming;
			size_t bytesWritten;
			std::string tempFile;
			size_t totalBytesReceived;
			bool chunkedComplete;

			ClientState() :
					state(IDLE),
					lastActivity(time(NULL)),
					contentLength(0),
					continueSent(false),
					keepAlive(true),
					isChunked(false),
					waitForEOF(false),
					response(200),
					isStreaming(false),
					bytesWritten(0),
					totalBytesReceived(0),
					chunkedComplete(false) {}
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
				isStreaming = false;
				chunkedComplete = false;
				totalBytesReceived = 0;
				continueSent = false;
				waitForEOF = false;
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
