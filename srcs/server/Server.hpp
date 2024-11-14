/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/06 19:55:53 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <set>
#include <string>
#include <cstdlib>
#include <sys/select.h>
#include "ServerConfig.hpp"
#include "../http/HTTPRequest.hpp"
#include "../http/Response.hpp"

class Server {
	public:
		// Constructor and Destructor
		explicit Server(int port, const std::string &host = "0.0.0.0");
		~Server();

		// Main server operations
		void start();
		void stop();

	private:
		// Server socket and configuration
		int _serverSocket;
		bool _isRunning;
		int _port;
		std::string _host;

		// File descriptor sets for select()
		fd_set _masterSet; // Tracks all file descriptors
		fd_set _readSet; // Monitors for read events
		fd_set _writeSet; // Monitors for write events
		int _maxFd;

		// Client state tracking
		struct ClientState {
			std::string requestBuffer;
			std::string responseBuffer;
			bool requestComplete;
			bool headersSent;
		};
		std::map<int, ClientState> _clients;

		// Socket setup methods
		bool initializeSocket();
		static void setNonBlocking(int sockfd);

		// Connection handling methods
		void handleNewConnection();
		void handleClientData(int clientFd);
		void handleClientWrite(int clientFd);
		void closeConnection(int clientFd);

		// Request/Response methods
		void processRequest(int clientFd, Server::ClientState &client);
		void sendResponse(int clientFd, ClientState &client);
		void sendErrorResponse(int clientFd, int statusCode, const std::string &message);

		// HTTP method handlers
		void handleGET(const std::string &path,
					   const std::map<std::string, std::string> &headers,
					   Response &response);
		void handlePOST(const std::string &path,
						const std::map<std::string, std::string> &headers,
						const std::string &body,
						Response &response);

		// Helper methods
		void updateMaxFd();
		static bool isRequestComplete(const std::string &request);
};

#endif
