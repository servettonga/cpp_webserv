/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:37:44 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "../server/SessionManager.hpp"
#include "CGIHandler.hpp"
#include "DirectoryHandler.hpp"
#include "FileHandler.hpp"

RequestHandler::RequestHandler(const ServerConfig &config) : _config(config) {
}

Response RequestHandler::handleRequest(const Request &request) {
	Request req = request;
	req.setConfig(&_config);
	const LocationConfig *location = getLocation(request.getPath());

	if (!location)
		return Response::makeErrorResponse(404, &_config);

	if (!location->redirect.empty()) {
		Response redirect(301); // Use 301 for permanent redirect as configured
		redirect.addHeader("Content-Type", "text/html");
		redirect.addHeader("Location", location->redirect);
		return redirect;
	}

	if (!isMethodAllowed(req.getMethod(), *location))
		return Response::makeErrorResponse(405, &_config);

	Response response;
	if (request.getMethod() == "GET")
		response = handleGET(request);
	else if (request.getMethod() == "POST")
		response = handlePOST(request);
	else if (request.getMethod() == "DELETE")
		response = handleDELETE(request);
	else if (request.getMethod() == "PUT")
		response = handlePUT(request);
	else
		response = Response::makeErrorResponse(501, &_config); // Not Implemented

	// Clean up expired sessions periodically
	static time_t lastCleanup = 0;
	if (time(NULL) - lastCleanup > 300) { // Every 5 minutes
		SessionManager::getInstance().cleanupExpiredSessions();
		lastCleanup = time(NULL);
	}
	handleCookies(request, response);
	return response;
}

const LocationConfig *RequestHandler::getLocation(const std::string &path) const {
	// First try regex patterns (including .bla files)
	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin(); it != _config.locations.end();
		 ++it) {
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
				if (pathEnd == pattern)
					return &(*it);
			}
		}
	}

	// Then try exact and prefix matches
	const LocationConfig *exactMatch = NULL;
	const LocationConfig *prefixMatch = NULL;
	size_t				  prefixLength = 0;

	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin(); it != _config.locations.end();
		 ++it) {
		if (it->path[0] == '~') // Skip regex locations
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
	for (std::vector<std::string>::const_iterator it = loc.methods.begin(); it != loc.methods.end(); ++it) {
		if (*it == method)
			return true;
	}
	return false;
}

Response RequestHandler::handleGET(const Request &request) const {
	std::string			  path = request.getPath();
	const LocationConfig *location = getLocation(path);
	if (!location)
		return Response::makeErrorResponse(404, &_config);

	std::string fullPath = FileHandler::constructFilePath(path, *location);
	// Check CGI for files
	size_t extPos = path.find_last_of('.');
	if (extPos != std::string::npos) {
		std::string										   ext = path.substr(extPos);
		std::map<std::string, std::string>::const_iterator handlerIt = _config.cgi_handlers.find(ext);

		if (handlerIt != _config.cgi_handlers.end()) {
			if (!location->cgi_path.empty()) { // Location has CGI configuration
				CGIHandler handler;
				Request	   req(request);
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
				return Response::makeErrorResponse(404, &_config);
		}
		return Response::makeErrorResponse(404, &_config);
	}
	// Handle directory
	if (S_ISDIR(st.st_mode)) {
		if (path != "/" && path != location->path) { // Check if directory access is allowed
			std::string locationRoot = location->root;
			bool		isListedPath = false;
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
				return Response::makeErrorResponse(403, &_config);
		}
		// First try the index file
		std::string indexPath =
			findFirstExistingIndex(fullPath, location->index.empty() ? _config.index : location->index);
		if (!indexPath.empty())
			return FileHandler::serveFile(indexPath, path);
		if (location->autoindex)
			return DirectoryHandler::handleDirectory(fullPath, *location, path, &_config);
		if (path == "/" || path == location->path) {
			Response response(200);
			response.addHeader("Content-Type", "text/html");
			return response;
		}
		return Response::makeErrorResponse(404, &_config);
	}
	return FileHandler::serveFile(fullPath, path);
}

Response RequestHandler::handlePOST(const Request &request) const {
	std::string			  path = request.getPath();
	const LocationConfig *location = getLocation(path);

	if (!location)
		return Response::makeErrorResponse(404, &_config);

	// Check body size limit (skip for zero-length body)
	if (!request.getBody().empty() && request.getBody().size() > location->client_max_body_size)
		return Response::makeErrorResponse(413, &_config);

	// CGI handling first
	size_t extPos = path.find_last_of('.');
	if (extPos != std::string::npos) {
		std::string										   ext = path.substr(extPos);
		std::map<std::string, std::string>::const_iterator handlerIt = _config.cgi_handlers.find(ext);
		if (handlerIt != _config.cgi_handlers.end()) {
			CGIHandler handler;
			return handler.executeCGI(request, handlerIt->second, FileHandler::constructFilePath(path, *location));
		}
	}

	// File upload handling
	const std::string &contentType = request.getHeader("Content-Type");
	if (contentType.find("multipart/form-data") != std::string::npos) {
		if (!location->path.empty())
			return FileHandler::handleFileUpload(request, *location);
		return Response::makeErrorResponse(403, &_config);
	}

	// Regular POST request
	Response response(200);
	response.addHeader("Content-Type", "text/plain");
	if (!request.getBody().empty())
		response.setBody(request.getBody());
	return response;
}

Response RequestHandler::handleDELETE(const Request &request) const {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return Response::makeErrorResponse(404, &_config);

	if (!isMethodAllowed("DELETE", *location))
		return Response::makeErrorResponse(405, &_config);

	return FileHandler::handleFileDelete(request, *location);
}

std::string RequestHandler::findFirstExistingIndex(const std::string &dirPath, const std::string &indexFiles) const {
	std::istringstream iss(indexFiles);
	std::string		   indexFile;

	while (iss >> indexFile) {
		if (!indexFile.empty() && indexFile[indexFile.length() - 1] == ';')
			indexFile = indexFile.substr(0, indexFile.length() - 1);
		std::string fullPath = dirPath + "/" + indexFile;
		struct stat st;
		if (stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
			return fullPath;
	}
	return "";
}

Response RequestHandler::handlePUT(const Request &request) const {
	// NOT IMPLEMENTED - Just returns OK without saving
	(void)request;
	Response response(200);
	response.addHeader("Content-Type", "text/plain");
	response.setBody("OK");
	return response;
}

void RequestHandler::handleCookies(const Request &request, Response &response) const {
	// Set server identification cookie
	response.setCookie("server", "webserv/1.0", "", "/");
	// Check for existing session
	std::map<std::string, std::string>			 cookies = request.getCookies();
	std::map<std::string, std::string>::iterator it = cookies.find("session_id");

	// Update visit count
	int											 visits = 1;
	std::map<std::string, std::string>::iterator visitsIt = cookies.find("visits");
	if (visitsIt != cookies.end())
		visits = std::atoi(visitsIt->second.c_str()) + 1;
	response.setCookie("visits", Utils::numToString(visits), "", "/");
	// If no session exists or session is invalid, create a new one
	if (it == cookies.end()) {
		response.setSessionId(SessionManager::getInstance().createSession()->getId());
		response.setCookie("visits", "1", "", "/");
	} else {
		SessionManager::getInstance().updateSession(it->second);
	}
}
