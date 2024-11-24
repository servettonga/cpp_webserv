/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/24 12:44:19 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "ServerConfig.hpp"
#include "../http/HTTPRequest.hpp"
#include "../http/Response.hpp"
#include <map>
#include <set>
#include <string>
#include <cstdlib>
#include <sys/select.h>

class Server {
	private:
		// Connection info
		std::string _host;
		int _port;
		int _serverSocket;
		bool _isRunning;

		// Socket sets
		fd_set _masterSet;
		fd_set _readSet;
		fd_set _writeSet;
		int _maxFd;

		// Client state tracking
		struct ClientState {
			std::string requestBuffer;
			std::string responseBuffer;
			bool requestComplete;
		};
		std::map<int, ClientState> _clients;

		// Socket setup
		bool initializeSocket();
		void setNonBlocking(int sockfd);

		// Connection handling
		void handleNewConnection();
		void handleClientData(int clientFd);
		void handleClientWrite(int clientFd);
		void closeConnection(int clientFd);

		// Request processing
		void processRequest(int clientFd, ClientState& client);
		bool isRequestComplete(const std::string& request);
		void sendErrorResponse(int clientFd, int statusCode, const std::string& message);
		std::string createDirectoryListing(const std::string &path, const std::string &urlPath);

		// HTTP method handlers
		void handleGET(const std::string& path, Response& response);
		void handlePOST(const std::string& path,
						const std::map<std::string, std::string>& headers,
						const std::string& body,
						Response& response);

	public:
		// Constructor and Destructor
		explicit Server(int port, const std::string &host = "0.0.0.0");
		~Server();

		void start();
		void stop();

};

#endif
