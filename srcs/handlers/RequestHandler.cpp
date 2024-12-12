/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/12 13:12:26 by sehosaf          ###   ########.fr       */
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

	if (!isMethodAllowed(req.getMethod(), *location)) {
		Response error(405);
		error.addHeader("Content-Type", "text/html");
		error.addHeader("Allow", "GET, POST");
		error.setBody("<html><body><h1>Method Not Allowed</h1></body></html>");
		return error;
	}

	if (req.getMethod() == "GET")
		return handleGET(request);
	else if (req.getMethod() == "POST")
		return handlePOST(request);
	else if (req.getMethod() == "DELETE")
		return handleDELETE(request);

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

	struct stat st;
	if (stat(fullPath.c_str(), &st) == 0) {
		// Check if this is a CGI request
		size_t extPos = path.find_last_of('.');
		if (extPos != std::string::npos) {
			std::string ext = path.substr(extPos);
			// First check global CGI handlers
			std::map<std::string, std::string>::const_iterator handlerIt = _config.cgi_handlers.find(ext);
			if (handlerIt != _config.cgi_handlers.end()) {
				HTTPRequest req(request);
				req.setConfig(&_config);
				CGIHandler handler;
				return handler.executeCGI(req, fullPath);
			}
			// Then check location's specific CGI handler
			if (!location->cgi_path.empty()) {
				HTTPRequest req(request);
				req.setConfig(&_config);
				CGIHandler handler;
				return handler.executeCGI(req, fullPath);
			}
		}

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
			return Response::makeErrorResponse(403);
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

	if (!isMethodAllowed("POST", *location))
		return Response::makeErrorResponse(405);

	std::string contentType = request.getHeader("Content-Type");
	std::cout << "POST Content-Type: " << contentType << std::endl;

	// Check if this is a CGI request
	std::string path = request.getPath();
	std::string fullPath = FileHandler::constructFilePath(path, *location);

	size_t extPos = path.find_last_of('.');
	if (extPos != std::string::npos) {
		std::string ext = path.substr(extPos);
		std::cout << "Checking CGI extension: " << ext << std::endl;

		// First check location's specific CGI handler
		if (!location->cgi_path.empty()) {
			std::cout << "Using location CGI handler: " << location->cgi_path << std::endl;
			HTTPRequest req(request);
			req.setConfig(&_config);
			CGIHandler handler;
			return handler.executeCGI(req, fullPath);
		}
		// Then fall back to global CGI handlers
		std::map<std::string, std::string>::const_iterator handlerIt = _config.cgi_handlers.find(ext);
		if (handlerIt != _config.cgi_handlers.end()) {
			std::cout << "Using global CGI handler for " << ext << ": " << handlerIt->second << std::endl;
			HTTPRequest req(request);
			req.setConfig(&_config);
			CGIHandler handler;
			return handler.executeCGI(req, fullPath);
		}
	}

	// Handle form submissions
	if (contentType == "application/x-www-form-urlencoded") {
		HTTPRequest req(request);
		req.setConfig(&_config);
		CGIHandler handler;
		return handler.executeCGI(req, fullPath);
	}

	// Handle file uploads
	if (contentType.find("multipart/form-data") != std::string::npos)
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