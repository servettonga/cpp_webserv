/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/26 17:24:13 by sehosaf          ###   ########.fr       */
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

Response RequestHandler::handleGET(const HTTPRequest& request) {
	// Security check
	if (!validatePath(request.getPath())) {
		return generateErrorResponse(403, "Forbidden");
	}

	// Get location config
	const LocationConfig *location = getLocation(request.getPath());
	if (!location) {
		return generateErrorResponse(404, "Not Found");
	}

	// Check method allowed
	if (!isMethodAllowed("GET", *location)) {
		return generateErrorResponse(405, "Method Not Allowed");
	}

	// Construct file path
	std::string fullPath = constructFilePath(request.getPath(), *location);

	// Handle directory or file
	struct stat st;
	if (stat(fullPath.c_str(), &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			return handleDirectory(fullPath, *location);
		} else {
			return serveStaticFile(fullPath, request.getPath());
		}
	}

	return generateErrorResponse(404, "Not Found");
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

Response RequestHandler::handleDirectory(const std::string &path, const LocationConfig &loc) {
	// Try index file first
	if (!loc.index.empty()) {
		std::string indexPath = path + "/" + loc.index;
		struct stat st;
		if (stat(indexPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode)) {
			return serveStaticFile(indexPath, "/index.html");
		}
	}

	// Generate listing if allowed
	if (loc.autoindex) {
		Response response(200);
		response.addHeader("Content-Type", "text/html");

		// Fix path calculation
		std::string relativePath;
		if (path == loc.root + "/files") {  // If in /files root
			relativePath = "";
		} else if (path.find(loc.root) == 0) {  // In subdirectory
			relativePath = path.substr(loc.root.length());
		}

		// Use location path directly
		std::string urlPath = loc.path;  // Should be just "/files"
		if (!relativePath.empty()) {
			if (relativePath[0] != '/')
				relativePath = "/" + relativePath;
			urlPath += relativePath;  // Add subdirectory path
		}

		response.setBody(createDirectoryListing(path, urlPath));
		return response;
	}
	return generateErrorResponse(403, "Forbidden");
}

bool RequestHandler::isMethodAllowed(const std::string& method, const LocationConfig& loc) {
	// Always allow GET by default
	if (loc.methods.empty() && method == "GET")
		return true;
	// Check against allowed methods
	for (std::vector<std::string>::const_iterator it = loc.methods.begin();
		 it != loc.methods.end(); ++it) {
		if (*it == method)
			return true;
	}
	return false;
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

bool RequestHandler::validatePath(const std::string& path) {
	// Check for path traversal attempts
	if (path.find("..") != std::string::npos ||
		path.find("//") != std::string::npos ||
		path.find("~") != std::string::npos) {
		return false;
	}
	// Ensure the path starts with /
	if (path.empty() || path[0] != '/')
		return false;
	return true;
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

	// Add parent directory link except for root directory
	if (urlPath != "/" && urlPath != "/files")
		html << "<tr><td><a href=\"../\">..</a></td><td>-</td><td>-</td></tr>\n";

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;

		struct stat statbuf;
		std::string fullPath = path + "/" + name;
		if (stat(fullPath.c_str(), &statbuf) == 0) {
			// Include full URL path in links
			html << "<tr><td><a href=\""
				 << (name == ".." ? "../" : urlPath + "/" + name);

			if (S_ISDIR(statbuf.st_mode) && name != "..")
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

Response RequestHandler::serveStaticFile(const std::string &fullPath, const std::string &urlPath) {
	std::ifstream file(fullPath.c_str(), std::ios::binary);
	if (!file.is_open())
		return generateErrorResponse(404, "Not Found");

	// Read file content
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();
	file.close();

	// Create response
	Response response(200);
	std::string mimeType = getContentType(urlPath);
	response.addHeader("Content-Type", mimeType);

	// Add Content-Disposition for downloads
	if (mimeType == "application/octet-stream" ||
		mimeType == "application/zip" ||
		mimeType == "application/x-executable") {
		response.addHeader("Content-Disposition",
						   "attachment; filename=\"" +
						   urlPath.substr(urlPath.find_last_of('/') + 1) + "\"");
	}

	response.setBody(content);
	return response;
}

Response RequestHandler::generateErrorResponse(int statusCode, const std::string& message) {
	Response response(statusCode);
	response.addHeader("Content-Type", "text/html");

	std::string body = "<html><body><h1>" +
					   message + "</h1></body></html>";
	response.setBody(body);
	return response;
}

std::string RequestHandler::constructFilePath(const std::string& uri, const LocationConfig& loc) {
	std::string path = loc.root;
	// Ensure root has trailing slash
	if (!path.empty() && path[path.length()-1] != '/')
		path += '/';
	// Add the URI path without a leading slash
	if (!uri.empty() && uri[0] == '/')
		path += uri.substr(1);
	else
		path += uri;
	return path;
}
