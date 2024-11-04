/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/04 22:11:55 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "../http/HTTPRequest.hpp"
#include "../http/Response.hpp"
#include "../server/ServerConfig.hpp"
#include "CGIHandler.hpp"
#include <map>
#include <string>

class RequestHandler {
	public:
		// Constructor and Destructor
		explicit RequestHandler(const ServerConfig &config);
		~RequestHandler();

		// Main handler method
		Response handleRequest(const HTTPRequest &request);

	private:
		// Configuration
		const ServerConfig &_config;
		CGIHandler _cgiHandler;

		// Method handlers
		Response handleGET(const HTTPRequest &request);
		Response handlePOST(const HTTPRequest &request);
		Response handleDELETE(const HTTPRequest &request);

		// Helper methods
		bool isMethodAllowed(const std::string &method, const LocationConfig &loc);
		bool isCGIRequest(const std::string &uri);
		Response serveStaticFile(const std::string &path);
		Response handleDirectory(const std::string &path);
		Response generateErrorResponse(int statusCode);
		std::string getContentType(const std::string &path);
		bool validatePath(const std::string &path);
		const LocationConfig *getLocation(const std::string &uri);
};

#endif
