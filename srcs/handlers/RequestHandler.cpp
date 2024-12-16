/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/16 12:38:33 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "FileHandler.hpp"
#include "DirectoryHandler.hpp"
#include "CGIHandler.hpp"
#include <sys/stat.h>
#include <cstring>
#include <sstream>
#include <iostream>

RequestHandler::RequestHandler(const ServerConfig &config) : _config(config) {}

Response RequestHandler::handleRequest(const HTTPRequest &request) {
	HTTPRequest req = request;
	req.setConfig(&_config);
	const LocationConfig *location = getLocation(request.getPath());

	// First check location exists
	if (!location)
		return Response::makeErrorResponse(404);

	// Then check method is allowed
	if (!isMethodAllowed(req.getMethod(), *location)) {
		Response error(405);
		error.addHeader("Content-Type", "text/html");
		std::string allow;
		for (std::vector<std::string>::const_iterator it = location->methods.begin();
			 it != location->methods.end(); ++it) {
			if (!allow.empty()) allow += ", ";
			allow += *it;
		}
		error.addHeader("Allow", allow);
		error.setBody("<html><body><h1>Method Not Allowed</h1></body></html>");
		return error;
	}

	// Handle methods
	if (req.getMethod() == "GET")
		return handleGET(request);
	else if (req.getMethod() == "POST")
		return handlePOST(request);
	else if (req.getMethod() == "DELETE")
		return handleDELETE(request);
	else if (req.getMethod() == "PUT")
		return handlePUT(request);

	return Response::makeErrorResponse(501);
}

const LocationConfig* RequestHandler::getLocation(const std::string& path) const {
	// First try regex patterns (including .bla files)
	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
		 it != _config.locations.end(); ++it) {
		if (!it->path.empty() && it->path[0] == '~') {
			size_t patternStart = it->path.find_first_not_of(" \t", 1);
			if (patternStart == std::string::npos)
				continue;

			std::string pattern = it->path.substr(patternStart);
			pattern = pattern.substr(0, pattern.find_first_of(" \t"));

			// Remove $ anchor if present
			bool hasEndAnchor = pattern[pattern.length() - 1] == '$';
			if (hasEndAnchor)
				pattern = pattern.substr(0, pattern.length() - 1);

			// Check if path matches pattern
			if (path.length() >= pattern.length()) {
				std::string pathEnd = path.substr(path.length() - pattern.length());
				if (pathEnd == pattern) {
					std::cout << "Matched regex pattern: " << pattern << " for path: " << path << std::endl;
					return &(*it);
				}
			}
		}
	}

	// Then try exact and prefix matches
	const LocationConfig* exactMatch = NULL;
	const LocationConfig* prefixMatch = NULL;
	size_t prefixLength = 0;

	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
		 it != _config.locations.end(); ++it) {
		if (it->path[0] == '~')  // Skip regex locations
			continue;
		if (it->path == path) {
			exactMatch = &(*it);
			break;
		}
		if (path.find(it->path) == 0 && it->path.length() > prefixLength) {
			prefixMatch = &(*it);
			prefixLength = it->path.length();
		}
	}

	return exactMatch ? exactMatch : prefixMatch;
}

bool RequestHandler::isMethodAllowed(const std::string &method, const LocationConfig &loc) const {
	for (std::vector<std::string>::const_iterator it = loc.methods.begin();
		 it != loc.methods.end(); ++it) {
		if (*it == method)
			return true;
	}
	return false;
}

Response RequestHandler::handleGET(const HTTPRequest &request) const {
	std::string path = request.getPath();
	const LocationConfig *location = getLocation(path);
	if (!location)
		return Response::makeErrorResponse(404);

	std::string fullPath = FileHandler::constructFilePath(path, *location);
	// Check CGI for files
	size_t extPos = path.find_last_of('.');
	if (extPos != std::string::npos) {
		std::string ext = path.substr(extPos);
		std::map<std::string, std::string>::const_iterator handlerIt =
				_config.cgi_handlers.find(ext);

		if (handlerIt != _config.cgi_handlers.end()) {
			if (!location->cgi_path.empty()) {  // Location has CGI configuration
				CGIHandler handler;
				HTTPRequest req(request);
				req.setConfig(&_config);
				return handler.executeCGI(req, handlerIt->second, fullPath);
			}
		}
	}
	struct stat st;
	if (stat(fullPath.c_str(), &st) != 0) {
		size_t lastSlash = fullPath.find_last_of('/');
		if (lastSlash != std::string::npos) {
			std::string parentDir = fullPath.substr(0, lastSlash);
			struct stat parentSt;
			if (stat(parentDir.c_str(), &parentSt) == 0 && S_ISDIR(parentSt.st_mode))
				return Response::makeErrorResponse(404);
		}
		return Response::makeErrorResponse(404);
	}
	// Handle directory
	if (S_ISDIR(st.st_mode)) {
		// Check if directory access is allowed
		if (path != "/" && path != location->path) {
			std::string locationRoot = location->root;
			bool isListedPath = false;
			if (fullPath.find(locationRoot) == 0) {
				isListedPath = true;
			} else {
				for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
					 it != _config.locations.end(); ++it) {
					if (path.find(it->path) == 0) {
						isListedPath = true;
						break;
					}
				}
			}
			if (!isListedPath)
				return Response::makeErrorResponse(403);
		}
		// First try index file
		std::string indexPath = findFirstExistingIndex(fullPath,
													   location->index.empty() ? _config.index : location->index);
		if (!indexPath.empty())
			return FileHandler::serveFile(indexPath, path);
		if (location->autoindex)
			return DirectoryHandler::handleDirectory(fullPath, *location, path);
		if (path == "/" || path == location->path) {
			Response response(200);
			response.addHeader("Content-Type", "text/html");
			return response;
		}
		return Response::makeErrorResponse(404);
	}
	return FileHandler::serveFile(fullPath, path);
}

Response RequestHandler::handlePOST(const HTTPRequest &request) const {
	std::string path = request.getPath();
	const LocationConfig *location = getLocation(path);

	if (!location)
		return Response::makeErrorResponse(404);
	if (request.getBody().size() > location->client_max_body_size)
		return Response::makeErrorResponse(413);

	// Check for CGI handlers first
	size_t extPos = path.find_last_of('.');
	if (extPos != std::string::npos) {
		std::string ext = path.substr(extPos);
		std::map<std::string, std::string>::const_iterator handlerIt =
				_config.cgi_handlers.find(ext);
		if (handlerIt != _config.cgi_handlers.end()) {
			std::string fullPath = FileHandler::constructFilePath(path, *location);
			CGIHandler handler;
			HTTPRequest req(request);
			req.setConfig(&_config);
			return handler.executeCGI(req, handlerIt->second, fullPath);
		}
	}

	// Handle other POST types
	const std::string& contentType = request.getHeader("Content-Type");
	if (contentType.find("multipart/form-data") != std::string::npos) {
		return FileHandler::handleFileUpload(request, *location);
	}

	// Remove content-type check for standard POST
	std::string fullPath = FileHandler::constructFilePath(path, *location);
	CGIHandler handler;
	HTTPRequest req(request);
	req.setConfig(&_config);
	std::map<std::string, std::string>::const_iterator cgiIt = _config.cgi_handlers.find(".bla");
	if (cgiIt != _config.cgi_handlers.end()) {
		return handler.executeCGI(req, cgiIt->second, fullPath);
	}

	return Response::makeErrorResponse(400);
}

Response RequestHandler::handleDELETE(const HTTPRequest &request) const {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return Response::makeErrorResponse(404);

	if (!isMethodAllowed("DELETE", *location))
		return Response::makeErrorResponse(405);

	return FileHandler::handleFileDelete(request, *location);
}

std::string RequestHandler::findFirstExistingIndex(const std::string& dirPath, const std::string& indexFiles) const {
	std::istringstream iss(indexFiles);
	std::string indexFile;

	while (iss >> indexFile) {
		if (!indexFile.empty() && indexFile[indexFile.length()-1] == ';')
			indexFile = indexFile.substr(0, indexFile.length()-1);
		std::string fullPath = dirPath + "/" + indexFile;
		struct stat st;
		if (stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
			return fullPath;
	}
	return "";
}

Response RequestHandler::handlePUT(const HTTPRequest &request) const {
	// Just return OK without saving
	(void)request;
	Response response(200);
	response.addHeader("Content-Type", "text/plain");
	response.setBody("OK");
	return response;
}
