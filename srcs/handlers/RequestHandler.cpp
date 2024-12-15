/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/15 11:42:32 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "FileHandler.hpp"
#include "DirectoryHandler.hpp"
#include "CGIHandler.hpp"
#include "../utils/Utils.hpp"
#include <sys/stat.h>
#include <cstring>
#include <sstream>
#include <iostream>

RequestHandler::RequestHandler(const ServerConfig &config) : _config(config) {}

Response RequestHandler::handleRequest(const HTTPRequest &request) {
	HTTPRequest req = request;
	req.setConfig(&_config);
	const LocationConfig *location = getLocation(request.getPath());
	if (!location) {
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

	// Check method permissions first
	if (!isMethodAllowed(req.getMethod(), *location)) {
		Response error(405);
		error.addHeader("Content-Type", "text/html");
		error.addHeader("Allow", "GET");
		error.setBody("<html><body><h1>Method Not Allowed</h1></body></html>");
		return error;
	}

	// Handle HEAD like GET but strip body
	if (req.getMethod() == "HEAD") {
		Response response = handleGET(request);
		response.setBody("");
		return response;
	}

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

const LocationConfig* RequestHandler::getLocation(const std::string &uri) const {
	const LocationConfig *bestMatch = NULL;
	size_t bestLength = 0;

	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin(); it != _config.locations.end(); ++it) {
		if (it->path == uri)
			return &(*it);
	}
	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
		 it != _config.locations.end(); ++it) {
		if (uri.find(it->path) == 0) {
			if (it->path.length() > bestLength) {
				bestMatch = &(*it);
				bestLength = it->path.length();
			}
		}
	}
	return bestMatch;
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
	size_t queryPos = path.find('?');
	if (queryPos != std::string::npos)
		path = path.substr(0, queryPos);

	if (!FileHandler::validatePath(path))
		return Response::makeErrorResponse(403);

	const LocationConfig *location = getLocation(path);
	if (!location)
		return Response::makeErrorResponse(404);

	std::string fullPath = FileHandler::constructFilePath(path, *location);

	// First check for CGI based on file extension
	size_t extPos = path.find_last_of('.');
	if (extPos != std::string::npos) {
		std::string ext = path.substr(extPos);

		// Check if the file exists first
		struct stat st;
		if (stat(fullPath.c_str(), &st) != 0) {
			return Response::makeErrorResponse(404);
		}

		// First check global CGI handlers
		std::map<std::string, std::string>::const_iterator handlerIt =
				_config.cgi_handlers.find(ext);

		if (handlerIt != _config.cgi_handlers.end()) {
			std::cout << "Found CGI handler for " << ext << ": " << handlerIt->second << std::endl;
			CGIHandler handler;
			HTTPRequest req(request);
			req.setConfig(&_config);
			return handler.executeCGI(req, handlerIt->second, fullPath);
		}
			// Then check location CGI handler
		else if (!location->cgi_path.empty()) {
			std::cout << "Using location CGI handler: " << location->cgi_path << std::endl;
			CGIHandler handler;
			HTTPRequest req(request);
			req.setConfig(&_config);
			return handler.executeCGI(req, location->cgi_path, fullPath);
		}
	}
	struct stat st;
	if (stat(fullPath.c_str(), &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			if (location->autoindex)
				return DirectoryHandler::handleDirectory(fullPath, *location, path);

			std::string indexFile = findFirstExistingIndex(
					fullPath,
					location->index.empty() ? _config.index : location->index
			);
			if (!indexFile.empty())
				return FileHandler::serveFile(indexFile, path + "/" +
														 indexFile.substr(indexFile.find_last_of('/') + 1));

			if (path == "/") {
				Response response(200);
				response.addHeader("Content-Type", "text/html");
				response.addHeader("Content-Length", "0");
				return response;
			}
			return Response::makeErrorResponse(404);
		}
		return FileHandler::serveFile(fullPath, path);
	}
	return Response::makeErrorResponse(404);
}

Response RequestHandler::handlePOST(const HTTPRequest &request) const {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return Response::makeErrorResponse(404);

	if (request.getBody().size() > location->client_max_body_size)
		return Response::makeErrorResponse(413);

	// Check for CGI first based on extension
	std::string path = request.getPath();
	size_t extPos = path.find_last_of('.');
	if (extPos != std::string::npos) {
		std::string ext = path.substr(extPos);
		std::map<std::string, std::string>::const_iterator handlerIt =
				_config.cgi_handlers.find(ext);

		if (handlerIt != _config.cgi_handlers.end()) {
			std::string fullPath = FileHandler::constructFilePath(path, *location);
			CGIHandler handler;
			return handler.executeCGI(request, handlerIt->second, fullPath);
		}
	}

	// Handle regular POST
	if (request.getHeader("Content-Type").find("multipart/form-data") != std::string::npos)
		return FileHandler::handleFileUpload(request, *location);

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
