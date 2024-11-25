/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/25 19:52:22 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include <dirent.h>     // For opendir, readdir, closedir, DIR
#include <sys/stat.h>   // For stat, struct stat
#include <ctime>		// For strftime
#include <sstream>		// For stringstream
#include <fstream>		// For ifstream, rdbuf
#include <iostream>
#include <cstring>

RequestHandler::RequestHandler(const ServerConfig &config) : _config(config) {}

RequestHandler::~RequestHandler() {}

Response RequestHandler::handleRequest(const HTTPRequest &request) {
	Response response(200);
	// Handle connection headers
	if (request.getVersion() == "HTTP/1.1") {
		std::string connection = request.getHeader("Connection");
		response.addHeader("Connection",
						   (connection == "close") ? "close" : "keep-alive");
	}
	// Route request
	if (request.getMethod() == "GET")
		return handleGET(request);
	else if (request.getMethod() == "POST")
		return handlePOST(request);
	response.setStatusCode(405);
	response.addHeader("Allow", "GET, POST");
	response.setBody("Method Not Allowed");
	return response;
}

Response RequestHandler::handleGET(const HTTPRequest &request) {
	Response response;
	std::string path = request.getPath();
	// Security check
	if (path.find("..") != std::string::npos || path.find('~') != std::string::npos) {
		response.setStatusCode(403);
		response.setBody("Forbidden");
		return response;
	}
	// Get matching location config
	const LocationConfig *location = getLocation(path);
	if (!location) {
		response.setStatusCode(404);
		response.setBody("Not Found");
		return response;
	}
	// Construct the full file path
	std::string fullPath;
	if (path == "/" || path == "/index.html")
		fullPath = location->root + "/index.html";
	else
		fullPath = location->root + path;
	// Check if the path is a directory
	struct stat fileStat;
	if (stat(fullPath.c_str(), &fileStat) == 0) {
		if (S_ISDIR(fileStat.st_mode)) {
			if (location->autoindex) {
				// Generate directory listing
				std::string listing = createDirectoryListing(fullPath, path);
				response.setStatusCode(200);
				response.addHeader("Content-Type", "text/html");
				response.setBody(listing);
				return response;
			} else if (!location->index.empty()) {
				fullPath += "/" + location->index;	// Try the index file
			} else {
				response.setStatusCode(403);
				response.setBody("Forbidden");
				return response;
			}
		}
	}
	// Serve a file
	std::ifstream file(fullPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		response.setStatusCode(404);
		response.setBody("Not Found");
		return response;
	}
	// Read file content
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();
	file.close();
	// Set response
	response.setStatusCode(200);
	std::string mimeType = getContentType(fullPath);
	response.addHeader("Content-Type", mimeType);
	// Add Content-Disposition for downloads
	if (mimeType == "application/octet-stream" ||
		mimeType == "application/zip" ||
		mimeType == "application/x-executable") {
		response.addHeader("Content-Disposition",
						   "attachment; filename=\"" + path.substr(path.find_last_of('/') + 1) + "\"");
	}
	response.setBody(content);
	return response;
}

Response RequestHandler::handlePOST(const HTTPRequest &request) {
	/*
		handlePOST(request):
		1. Validate request path
		2. Check content length limits
		3. IF CGI request:
		   - Route to CGI handler
		4. ELSE IF file upload:
		   - Validate file type
		   - Store file
		   - Return success response
		5. ELSE:
		   Return 400 Bad Request
	*/
	(void)request;
	return Response();
}

Response RequestHandler::handleDELETE(const HTTPRequest &request) {
	/*
		handleDELETE(request):
		1. Validate request path
		2. Check permissions
		3. IF file exists:
		   - Delete file
		   - Return success response
		4. ELSE:
		   Return 404 Not Found
	*/
	(void)request;
	return Response();
}

bool RequestHandler::isMethodAllowed(const std::string &method, const LocationConfig &loc) {
	/*
		isMethodAllowed(method, location):
		1. Get allowed methods from location config
		2. Check if method is in allowed list
		3. Return result
	*/
	(void)method;
	(void)loc;
	return false;
}

bool RequestHandler::isCGIRequest(const std::string &uri) {
	/*
		isCGIRequest(uri):
		1. Get file extension
		2. Check against CGI extensions
		3. Return result
	*/
	(void)uri;
	return false;
}

Response RequestHandler::serveStaticFile(const std::string &path) {
	/*
		serveStaticFile(path):
		1. Open file
		2. Read content
		3. Get content type
		4. Create response with file content
		5. Return response
	*/
	(void)path;
	return Response();
}

Response RequestHandler::handleDirectory(const std::string &path) {
	/*
		handleDirectory(path):
		1. Check for index file
		2. IF index exists:
		   - Serve index file
		3. ELSE IF autoindex enabled:
		   - Generate directory listing
		4. ELSE:
		   - Return 403 Forbidden
	*/
	(void)path;
	return Response();
}

Response RequestHandler::generateErrorResponse(int statusCode) {
	/*
		generateErrorResponse(statusCode):
		1. Check for custom error page
		2. IF custom page exists:
		   - Serve custom error page
		3. ELSE:
		   - Generate default error response
	*/
	(void)statusCode;
	return Response();
}

std::string RequestHandler::getContentType(const std::string& path) {
	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos)
		return "application/octet-stream";

	std::string ext = path.substr(dot + 1);

	// Text files
	if (ext == "html" || ext == "htm") return "text/html";
	if (ext == "css") return "text/css";
	if (ext == "js") return "application/javascript";
	if (ext == "json") return "application/json";
	if (ext == "txt") return "text/plain";
	if (ext == "xml") return "application/xml";

	// Images
	if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
	if (ext == "png") return "image/png";
	if (ext == "gif") return "image/gif";
	if (ext == "svg") return "image/svg+xml";
	if (ext == "ico") return "image/x-icon";

	// Documents
	if (ext == "pdf") return "application/pdf";

	// Binary/Compressed
	if (ext == "zip") return "application/zip";
	if (ext == "tar") return "application/x-tar";
	if (ext == "gz") return "application/gzip";

	return "application/octet-stream";
}

bool RequestHandler::validatePath(const std::string &path) {
	/*
		validatePath(path):
		1. Check for path traversal
		2. Verify path is within root
		3. Check file exists
		4. Check permissions
		5. Return validation result
	*/
	(void)path;
	return false;
}

const LocationConfig* RequestHandler::getLocation(const std::string& uri) {
	const LocationConfig* bestMatch = NULL;
	size_t bestLength = 0;

	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
		 it != _config.locations.end(); ++it) {
		// Special handling for root location
		if (it->path == "/") {
			if (!bestMatch) {
				bestMatch = &(*it);
				bestLength = 1;
			}
			continue;
		}
		// For other locations, check if URI starts with location path
		if (uri.find(it->path) == 0) {
			if (it->path.length() > bestLength) {
				bestMatch = &(*it);
				bestLength = it->path.length();
			}
		}
	}
	return bestMatch;
}

std::string RequestHandler::createDirectoryListing(const std::string &path, const std::string &urlPath) {
	DIR* dir = opendir(path.c_str());
	if (!dir)
		return "";
	std::stringstream html;
	html << "<html><head><title>Directory: " << urlPath << "</title></head><body>\n"
		 << "<h1>Directory: " << urlPath << "</h1>\n"
		 << "<table>\n"
		 << "<tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n";
	// Add the parent directory link only if not in root
	if (urlPath != "/files/")
		html << "<tr><td><a href=\"../\">..</a></td><td>-</td><td>-</td></tr>\n";
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		struct stat statbuf;
		std::string fullPath = path + "/" + name;
		if (stat(fullPath.c_str(), &statbuf) == 0) {
			// Remove trailing slash from urlPath if exists
			std::string cleanUrlPath = urlPath;
			if (cleanUrlPath[cleanUrlPath.length() - 1] == '/')
				cleanUrlPath = cleanUrlPath.substr(0, cleanUrlPath.length() - 1);
			html << "<tr><td><a href=\"";
			if (urlPath == "/files/")
				html << "/files/" << name;
			else
				html << cleanUrlPath << "/" << name;
			// Add trailing slash only for directories
			if (S_ISDIR(statbuf.st_mode))
				html << "/";
			html << "\">" << name << "</a></td>";
			if (S_ISDIR(statbuf.st_mode))
				html << "<td>-</td>";
			else
				html << "<td>" << statbuf.st_size << " bytes</td>";
			char timebuf[32];
			strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
					 localtime(&statbuf.st_mtime));
			html << "<td>" << timebuf << "</td></tr>\n";
		}
	}
	closedir(dir);
	html << "</table></body></html>";
	return html.str();
}
