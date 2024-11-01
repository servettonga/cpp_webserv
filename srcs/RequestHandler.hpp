/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 19:44:01 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include "ConfigParser.hpp"
#include "HTTPRequest.hpp"
#include "Response.hpp"

// RequestHandler class is responsible for handling incoming HTTP requests
class RequestHandler {
	public:
		explicit RequestHandler(const std::vector<ServerConfig>& serverConfigs);
		~RequestHandler();

		void handleRequest(const std::string& requestStr, int clientFd);

	private:
		const std::vector<ServerConfig>& serverConfigs;

		// Helper methods
		const ServerConfig& findServerConfig(const HTTPRequest& request);
		const LocationConfig& findLocationConfig(const HTTPRequest& request, const ServerConfig& serverConfig);
		bool isMethodAllowed(const std::string& method, const LocationConfig& locationConfig);

		// Method handlers
		void handleGetRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& locationConfig);
		void handlePostRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& locationConfig);
		void handleDeleteRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& locationConfig);

		// Error response
		void sendErrorResponse(int clientFd, int statusCode, const ServerConfig& serverConfig);

		// Utility methods
		static std::string getReasonPhrase(int statusCode);
		static std::string readFileContent(const std::string& filePath);
		static std::string constructFilePath(const std::string& uri, const LocationConfig& locationConfig);
		static std::string sanitizeURI(const std::string& uri);
		static std::string urlDecode(const std::string& str);
		static std::string intToString(int value);

		// Additional helper methods as needed
};

#endif // REQUESTHANDLER_HPP
