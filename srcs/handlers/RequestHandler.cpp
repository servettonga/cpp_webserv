/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/03 14:15:13 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "FileHandler.hpp"
#include "DirectoryHandler.hpp"
#include <sys/stat.h>
#include <cstring>

RequestHandler::RequestHandler(const ServerConfig &config) : _config(config) {}

Response RequestHandler::handleRequest(const HTTPRequest &request) {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location) {
		// Get the error page path from config
		std::map<int, std::string>::const_iterator it = _config.error_pages.find(404);
		if (it != _config.error_pages.end()) {
			std::string fullPath = _config.root + it->second;
			struct stat st;
			int statResult = stat(fullPath.c_str(), &st);
			if (statResult == 0) {
				if (!S_ISDIR(st.st_mode)) {
					Response response = FileHandler::serveFile(fullPath, it->second);
					response.setStatusCode(404);
					return response;
				}
			}
		}
		return Response::makeErrorResponse(404);
	}

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

	return Response::makeErrorResponse(501);
}

const LocationConfig* RequestHandler::getLocation(const std::string &uri) const {
	const LocationConfig* bestMatch = NULL;
	size_t bestLength = 0;

	// First try exact matches
	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
		 it != _config.locations.end(); ++it) {
		if (it->path == uri)
			return &(*it);
	}

	// Then find the longest matching prefix
	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
		 it != _config.locations.end(); ++it) {
		// Root location should be the last resort
		if (it->path == "/") {
			if (!bestMatch) {
				bestMatch = &(*it);
				bestLength = 1;
			}
			continue;
		}

		// Check if URI starts with this location's path
		if (uri.find(it->path) == 0) {
			// Only match complete path segments
			if (uri.length() == it->path.length() ||
				uri[it->path.length()] == '/') {
				if (it->path.length() > bestLength) {
					bestMatch = &(*it);
					bestLength = it->path.length();
				}
			}
		}
	}

	// Return NULL for unmatched paths, even if root location exists
	if (bestMatch && bestMatch->path == "/" && uri != "/")
		return NULL;

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

Response RequestHandler::handleGET(const HTTPRequest &request) const {
	if (!FileHandler::validatePath(request.getPath()))
		return Response::makeErrorResponse(403);

	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return Response::makeErrorResponse(404);

	std::string fullPath = FileHandler::constructFilePath(request.getPath(), *location);

	struct stat st;
	if (stat(fullPath.c_str(), &st) == 0) {
		if (S_ISDIR(st.st_mode))
			return DirectoryHandler::handleDirectory(fullPath, *location);
		else
			return FileHandler::serveFile(fullPath, request.getPath());
	}

	return Response::makeErrorResponse(404);
}

Response RequestHandler::handlePOST(const HTTPRequest &request) const {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return Response::makeErrorResponse(404);

	// Check body size against location limit
	if (request.getBody().size() > location->client_max_body_size)
		return Response::makeErrorResponse(413);

	if (!isMethodAllowed("POST", *location))
		return Response::makeErrorResponse(405);

	std::string contentType = request.getHeader("Content-Type");
	if (contentType.find("multipart/form-data") == std::string::npos)
		return Response::makeErrorResponse(415);

	return FileHandler::handleFileUpload(request, *location);
}

Response RequestHandler::handleDELETE(const HTTPRequest &request) const {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return Response::makeErrorResponse(404);

	if (!isMethodAllowed("DELETE", *location))
		return Response::makeErrorResponse(405);

	return FileHandler::handleFileDelete(request, *location);
}
