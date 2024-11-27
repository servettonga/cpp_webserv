/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/27 12:07:29 by sehosaf          ###   ########.fr       */
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
#include <csignal>

RequestHandler::RequestHandler(const ServerConfig &config) : _config(config) {}

RequestHandler::~RequestHandler() {}

Response RequestHandler::handleRequest(const HTTPRequest &request) {
	// Get location config first
	const LocationConfig *location = getLocation(request.getPath());
	if (!location) {
		std::cout << "No location found for path: " << request.getPath() << std::endl;
		return generateErrorResponse(404, "Not Found");
	}

	std::cout << "Found location: " << location->path << std::endl;
	std::cout << "Request method: " << request.getMethod() << std::endl;

	// Check method allowed
	if (!isMethodAllowed(request.getMethod(), *location)) {
		std::cout << "Method not allowed: " << request.getMethod() << std::endl;
		Response error(405);
		error.addHeader("Content-Type", "text/html");
		error.addHeader("Allow", "GET, POST");
		error.setBody("<html><body><h1>Method Not Allowed</h1></body></html>");
		return error;
	}

	// Route request
	if (request.getMethod() == "GET")
		return handleGET(request);
	else if (request.getMethod() == "POST")
		return handlePOST(request);

	return generateErrorResponse(501, "Not Implemented");
}

Response RequestHandler::handleGET(const HTTPRequest& request) {
	// Security check
	if (!validatePath(request.getPath()))
		return generateErrorResponse(403, "Forbidden");

	// Get location config
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return generateErrorResponse(404, "Not Found");

	// Check method allowed
	if (!isMethodAllowed("GET", *location))
		return generateErrorResponse(405, "Method Not Allowed");

	// Construct file path
	std::string fullPath = constructFilePath(request.getPath(), *location);

	// Handle directory or file
	struct stat st;
	if (stat(fullPath.c_str(), &st) == 0) {
		if (S_ISDIR(st.st_mode))
			return handleDirectory(fullPath, *location);
		else
			return serveStaticFile(fullPath, request.getPath());
	}

	return generateErrorResponse(404, "Not Found");
}

Response RequestHandler::handlePOST(const HTTPRequest &request) {
	const LocationConfig *location = getLocation(request.getPath());
	if (!location)
		return generateErrorResponse(404, "Not Found");

	if (!isMethodAllowed("POST", *location))
		return generateErrorResponse(405, "Method Not Allowed");

	// Get boundary from Content-Type
	std::string contentType = request.getHeader("Content-Type");
	size_t boundaryPos = contentType.find("boundary=");
	if (boundaryPos == std::string::npos)
		return generateErrorResponse(400, "Bad Request - No boundary");

	std::string boundary = "--" + contentType.substr(boundaryPos + 9);
	std::string body = request.getBody();

	// Parse multipart data
	size_t startPos = body.find(boundary);
	if (startPos == std::string::npos)
		return generateErrorResponse(400, "Bad Request - Invalid multipart data");

	// Find Content-Disposition header
	size_t headerStart = body.find("Content-Disposition:", startPos);
	if (headerStart == std::string::npos)
		return generateErrorResponse(400, "Bad Request - No Content-Disposition");

	// Extract filename
	size_t filenamePos = body.find("filename=\"", headerStart);
	if (filenamePos == std::string::npos)
		return generateErrorResponse(400, "Bad Request - No filename");

	size_t filenameEnd = body.find("\"", filenamePos + 10);
	std::string filename = body.substr(filenamePos + 10, filenameEnd - (filenamePos + 10));

	// Find content start
	size_t contentStart = body.find("\r\n\r\n", filenameEnd) + 4;
	size_t contentEnd = body.find(boundary, contentStart) - 2; // -2 for \r\n
	std::string content = body.substr(contentStart, contentEnd - contentStart);

	// Save file
	std::string filepath = location->root + "/" + filename;
	std::ofstream outFile(filepath.c_str(), std::ios::binary);
	if (!outFile)
		return generateErrorResponse(500, "Internal Server Error - Cannot create file");

	outFile.write(content.c_str(), content.length());
	outFile.close();

	// Return success
	Response response(201);
	response.addHeader("Content-Type", "text/plain");
	response.setBody("File uploaded successfully");
	return response;
}

std::vector<FormDataPart> RequestHandler::parseMultipartFormData(const HTTPRequest& request) {
	std::vector<FormDataPart> parts;
	std::string contentType = request.getHeader("Content-Type");

	// Debug output
	std::cout << "Parsing multipart data" << std::endl;
	std::cout << "Content-Type full: " << contentType << std::endl;

	size_t boundaryPos = contentType.find("boundary=");
	if (boundaryPos == std::string::npos) {
		std::cout << "No boundary found" << std::endl;
		return parts;
	}

	std::string boundary = "--" + contentType.substr(boundaryPos + 9);

	// Debug boundary
	std::cout << "Boundary: " << boundary << std::endl;

	std::string body = request.getBody();
	size_t pos = 0;

	while ((pos = body.find(boundary, pos)) != std::string::npos) {
		pos += boundary.length();

		// Skip leading \r\n
		if (pos < body.length() && body[pos] == '\r') pos++;
		if (pos < body.length() && body[pos] == '\n') pos++;

		// Find next boundary
		size_t nextBoundary = body.find(boundary, pos);
		if (nextBoundary == std::string::npos) break;

		// Extract part content
		std::string part = body.substr(pos, nextBoundary - pos);

		// Debug part
		std::cout << "Found part, length: " << part.length() << std::endl;

		FormDataPart formPart;
		size_t headerEnd = part.find("\r\n\r\n");
		if (headerEnd != std::string::npos) {
			// Parse headers
			std::string headers = part.substr(0, headerEnd);
			std::cout << "Headers: " << headers << std::endl;

			size_t start = 0, end;
			while ((end = headers.find("\r\n", start)) != std::string::npos) {
				std::string header = headers.substr(start, end - start);
				size_t colon = header.find(": ");
				if (colon != std::string::npos) {
					std::string name = header.substr(0, colon);
					std::string value = header.substr(colon + 2);
					formPart.headers[name] = value;
					std::cout << "Header: " << name << " = " << value << std::endl;
				}
				start = end + 2;
			}

			// Get content
			formPart.content = part.substr(headerEnd + 4);
			std::cout << "Content size: " << formPart.content.size() << std::endl;

			parts.push_back(formPart);
		}
	}

	std::cout << "Found " << parts.size() << " parts" << std::endl;
	return parts;
}

std::string RequestHandler::sanitizeFilename(const std::string &filename) {
	std::string safe;
	for (size_t i = 0; i < filename.length(); i++) {
		char c = filename[i];
		if (isalnum(c) || c == '-' || c == '_' || c == '.')
			safe += c;
	}
	return safe;
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

/*
 * @brief Handle directory listing for the given path and location
 */
Response RequestHandler::handleDirectory(const std::string &path, const LocationConfig &loc) {
	// Try the index file first
	if (!loc.index.empty()) {
		std::string indexPath = path + "/" + loc.index;
		struct stat st;
		if (stat(indexPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
			return serveStaticFile(indexPath, "/index.html");
	}

	// Generate listing if allowed
	if (loc.autoindex) {
		Response response(200);
		response.addHeader("Content-Type", "text/html");
		// Start with the base location path
		std::string urlPath = loc.path;
		// If we're in a subdirectory, extract only the part after loc.path
		if (path.length() > loc.root.length()) {
			std::string subPath = path.substr(loc.root.length());
			if (subPath.substr(0, loc.path.length()) == loc.path)
				subPath = subPath.substr(loc.path.length());
			if (!subPath.empty()) {
				if (subPath[0] != '/')
					subPath = "/" + subPath;
				urlPath += subPath;
			}
		}
		response.setBody(createDirectoryListing(path, urlPath));
		return response;
	}

	return generateErrorResponse(403, "Forbidden");
}

/*
 * @brief Create an HTML directory listing for the given path and URL path
 */
std::string RequestHandler::createDirectoryListing(const std::string &path, const std::string &urlPath) {
	DIR* dir = opendir(path.c_str());
	if (!dir)
		return "";

	std::stringstream html;
	html << "<html><head><title>Directory: " << urlPath << "</title></head><body>\n"
		 << "<h1>Directory: " << urlPath << "</h1>\n"
		 << "<table>\n"
		 << "<tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n";
	if (urlPath != "/")
		html << "<tr><td><a href=\"../\">..</a></td><td>-</td><td>-</td></tr>\n";
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		struct stat statbuf;
		std::string fullPath = path + "/" + name;
		if (stat(fullPath.c_str(), &statbuf) == 0) {
			std::string linkPath = urlPath;
			if (!urlPath.empty() && urlPath[urlPath.length() - 1] != '/')
				linkPath += '/';
			linkPath += name;
			html << "<tr><td><a href=\"" << linkPath;
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

bool RequestHandler::isMethodAllowed(const std::string& method, const LocationConfig& loc) {
	// Debug output
	std::cout << "Checking if method '" << method << "' is allowed in location " << loc.path << std::endl;
	std::cout << "Location has " << loc.methods.size() << " allowed methods:" << std::endl;
	for (std::vector<std::string>::const_iterator it = loc.methods.begin();
		 it != loc.methods.end(); ++it) {
		std::cout << "- " << *it << std::endl;
	}

	// Validate method is not empty
	if (method.empty()) {
		std::cout << "Empty method provided" << std::endl;
		return false;
	}

	// Check if we're in uploads location and method is POST
	if (loc.path == "/files/uploads" && method == "POST") {
		std::cout << "POST allowed in uploads location" << std::endl;
		return true;
	}

	// Check against allowed methods
	for (std::vector<std::string>::const_iterator it = loc.methods.begin();
		 it != loc.methods.end(); ++it) {
		if (*it == method) {
			std::cout << "Method " << method << " found in allowed methods" << std::endl;
			return true;
		}
	}

	std::cout << "Method " << method << " not allowed" << std::endl;
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

	std::cout << "Looking for location matching URI: " << uri << std::endl;

	for (std::vector<LocationConfig>::const_iterator it = _config.locations.begin();
		 it != _config.locations.end(); ++it) {
		std::cout << "Checking location: " << it->path << std::endl;
		std::cout << "Methods: ";
		for (std::vector<std::string>::const_iterator mit = it->methods.begin();
			 mit != it->methods.end(); ++mit) {
			std::cout << *mit << " ";
		}
		std::cout << std::endl;

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

	if (bestMatch) {
		std::cout << "Found matching location: " << bestMatch->path << std::endl;
	}

	return bestMatch;
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
	Response response(200);	// Create response with 200 OK
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

Response RequestHandler::generateErrorResponse(int statusCode, const std::string &message) {
	Response response(statusCode);
	response.addHeader("Content-Type", "text/html");
	std::string body = "<html><body><h1>" +
					   message + "</h1></body></html>";
	response.setBody(body);
	return response;
}

std::string RequestHandler::constructFilePath(const std::string &uri, const LocationConfig &loc) {
	std::string path = loc.root;
	if (!path.empty() && path[path.length()-1] != '/')	// Ensure root has trailing slash
		path += '/';
	if (!uri.empty() && uri[0] == '/')	// Add the URI path without a leading slash
		path += uri.substr(1);
	else
		path += uri;
	return path;
}
