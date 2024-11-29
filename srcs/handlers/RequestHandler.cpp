/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 21:31:42 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "FileHandler.hpp"
#include "DirectoryHandler.hpp"
#include <sys/stat.h>

RequestHandler::RequestHandler(const ServerConfig &config) : _config(config) {}

Response RequestHandler::handleRequest(const HTTPRequest &request) {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return generateErrorResponse(404, "Not Found");

	if (!isMethodAllowed(request.getMethod(), *location)) {
		Response error(405);
		error.addHeader("Content-Type", "text/html");
		error.addHeader("Allow", "GET, POST");
		error.setBody("<html><body><h1>Method Not Allowed</h1></body></html>");
		return error;
	}

	if (request.getMethod() == "GET")
		return handleGET(request);
	else if (request.getMethod() == "POST")
		return handlePOST(request);
	else if (request.getMethod() == "DELETE")
		return handleDELETE(request);

	return generateErrorResponse(501, "Not Implemented");
}

const LocationConfig* RequestHandler::getLocation(const std::string &uri) const {
	const LocationConfig* bestMatch = NULL;
	size_t bestLength = 0;

	if (uri == "/") {
		for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
			 it != _config.locations.end(); ++it) {
			if (it->path == "/")
				return &(*it);
		}
	}

	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
		 it != _config.locations.end(); ++it) {
		if (uri.find(it->path) == 0 && it->path.length() > bestLength) {
			bestMatch = &(*it);
			bestLength = it->path.length();
		}
	}

	return bestMatch;
}

bool RequestHandler::isMethodAllowed(const std::string &method, const LocationConfig &loc) const {
	if (method.empty())
		return false;

	for (std::vector<std::string>::const_iterator it = loc.methods.begin();
		 it != loc.methods.end(); ++it) {
		if (*it == method)
			return true;
	}
	return false;
}

Response RequestHandler::generateErrorResponse(int statusCode, const std::string &message) const {
	Response response(statusCode);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>" + message + "</h1></body></html>");
	return response;
}

Response RequestHandler::handleGET(const HTTPRequest &request) const {
	if (!FileHandler::validatePath(request.getPath()))
		return generateErrorResponse(403, "Forbidden");

	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return generateErrorResponse(404, "Not Found");

	std::string fullPath = FileHandler::constructFilePath(request.getPath(), *location);

	struct stat st;
	if (stat(fullPath.c_str(), &st) == 0) {
		if (S_ISDIR(st.st_mode))
			return DirectoryHandler::handleDirectory(fullPath, *location);
		else
			return FileHandler::serveFile(fullPath, request.getPath());
	}

	return generateErrorResponse(404, "Not Found");
}

Response RequestHandler::handlePOST(const HTTPRequest &request) const {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return generateErrorResponse(404, "Not Found");

	if (!isMethodAllowed("POST", *location))
		return generateErrorResponse(405, "Method Not Allowed");

	// File upload handling
	std::string contentType = request.getHeader("Content-Type");
	if (contentType.find("multipart/form-data") == std::string::npos)
		return generateErrorResponse(415, "Unsupported Media Type");

	return FileHandler::handleFileUpload(request, *location);
}

Response RequestHandler::handleDELETE(const HTTPRequest &request) const {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return generateErrorResponse(404, "Not Found");

	if (!isMethodAllowed("DELETE", *location))
		return generateErrorResponse(405, "Method Not Allowed");

	return FileHandler::handleFileDelete(request, *location);
}
