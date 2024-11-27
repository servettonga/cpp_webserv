/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/26 23:19:44 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "../http/HTTPRequest.hpp"
#include "../http/Response.hpp"
#include "../server/ServerConfig.hpp"
#include "FormDataPart.hpp"
#include "CGIHandler.hpp"
#include <map>
#include <string>

class RequestHandler {
	private:
		// Core request handling
		Response handleGET(const HTTPRequest &request);
		Response handlePOST(const HTTPRequest &request);
		Response handleDELETE(const HTTPRequest &request);

		// Location handling
		const LocationConfig* getLocation(const std::string &uri);
		bool isMethodAllowed(const std::string& method, const LocationConfig &loc);

		// File handling
		Response serveStaticFile(const std::string& fullPath, const std::string &urlPath);
		Response handleDirectory(const std::string& path, const LocationConfig &loc);
		std::string createDirectoryListing(const std::string& path, const std::string &urlPath);
		std::vector<FormDataPart> parseMultipartFormData(const HTTPRequest& request);

		// Helper methods
		std::string getContentType(const std::string &path);
		bool validatePath(const std::string &path);
		Response generateErrorResponse(int statusCode, const std::string &message);
		std::string constructFilePath(const std::string& uri, const LocationConfig &loc);
		static std::string sanitizeFilename(const std::string& filename);
		std::string urlDecode(const std::string& encoded);

		// Member variables
		const ServerConfig &_config;

	public:
		explicit RequestHandler(const ServerConfig &config);
		~RequestHandler();

		Response handleRequest(const HTTPRequest &request);
};

#endif
