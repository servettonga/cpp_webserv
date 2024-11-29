/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:09:27 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 22:42:42 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FileHandler.hpp"
#include "MimeTypes.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

Response FileHandler::serveFile(const std::string &path, const std::string &urlPath) {
	if (!isValidFilePath(path))
		return Response(403, "Forbidden");

	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return Response(404, "Not Found");

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	Response response(200);
	response.addHeader("Content-Type", MimeTypes::getType(urlPath));
	response.setBody(buffer.str());

	return response;
}

Response FileHandler::handleFileUpload(const HTTPRequest &request, const LocationConfig &loc) {
	std::string boundary = extractBoundary(request.getHeader("Content-Type"));
	if (boundary.empty())
		return Response(400, "Bad Request - Invalid Content-Type");

	FileData fileData = parseMultipartData(request.getBody(), boundary);
	if (!fileData.isValid)
		return Response(400, "Bad Request - Invalid file data");

	std::string filepath = constructUploadPath(loc, fileData.filename);
	if (!saveUploadedFile(filepath, fileData.content))
		return Response(500, "Internal Server Error - File save failed");

	return createSuccessResponse();
}

std::string FileHandler::extractBoundary(const std::string &contentType) {
	size_t boundaryPos = contentType.find("boundary=");
	return (boundaryPos != std::string::npos) ?
		   "--" + contentType.substr(boundaryPos + 9) : "";
}

FileHandler::FileData FileHandler::parseMultipartData(const std::string &body, const std::string &boundary) {
	FileData data;
	size_t partStart = body.find(boundary);
	size_t headerStart = body.find("Content-Disposition:", partStart);
	size_t filenamePos = body.find("filename=\"", headerStart);

	if (partStart == std::string::npos || headerStart == std::string::npos ||
		filenamePos == std::string::npos)
		return data;

	size_t filenameEnd = body.find("\"", filenamePos + 10);
	data.filename = sanitizeFilename(
			body.substr(filenamePos + 10, filenameEnd - (filenamePos + 10))
	);

	size_t contentStart = body.find("\r\n\r\n", filenameEnd) + 4;
	size_t contentEnd = body.find(boundary, contentStart) - 2;
	data.content = body.substr(contentStart, contentEnd - contentStart);
	data.isValid = true;

	return data;
}

std::string FileHandler::constructUploadPath(const LocationConfig &loc, const std::string &filename) {
	std::string path = loc.root + loc.path;
	return path + (path[path.length() - 1] != '/' ? "/" : "") + filename;
}

bool FileHandler::saveUploadedFile(const std::string &filepath, const std::string &content) {
	const size_t CHUNK_SIZE = 1024 * 1024;  // 1MB chunks
	std::ofstream file(filepath.c_str(), std::ios::binary);

	if (!file.is_open())
		return false;

	size_t remaining = content.length();
	size_t offset = 0;

	while (remaining > 0) {
		size_t chunk = std::min(remaining, CHUNK_SIZE);
		file.write(content.c_str() + offset, chunk);

		offset += chunk;
		remaining -= chunk;
	}

	file.close();
	return true;
}

Response FileHandler::createSuccessResponse() {
	Response response(201);
	response.addHeader("Content-Type", "text/plain");
	response.setBody("File uploaded successfully");
	return response;
}

Response FileHandler::handleFileDelete(const HTTPRequest &request, const LocationConfig &loc) {
	std::string filepath = constructFilePath(request.getPath(), loc);

	struct stat st;
	if (stat(filepath.c_str(), &st) != 0)
		return Response(404, "Not Found");

	if (S_ISDIR(st.st_mode))
		return Response(403, "Forbidden - Cannot delete directories");

	if (unlink(filepath.c_str()) != 0)
		return Response(500, "Internal Server Error");

	Response response(200);
	response.setBody("File deleted successfully");
	return response;
}

bool FileHandler::validatePath(const std::string &path) {
	return path.find("..") == std::string::npos;
}

std::string FileHandler::urlDecode(const std::string &encoded) {
	std::string decoded;
	for (size_t i = 0; i < encoded.length(); ++i) {
		if (encoded[i] == '%' && i + 2 < encoded.length()) {
			int value;
			std::istringstream iss(encoded.substr(i + 1, 2));
			if (iss >> std::hex >> value) {
				decoded += static_cast<char>(value);
				i += 2;
			} else {
				decoded += encoded[i];
			}
		} else if (encoded[i] == '+') {
			decoded += ' ';
		} else {
			decoded += encoded[i];
		}
	}
	return decoded;
}

std::string FileHandler::sanitizeFilename(const std::string &filename) {
	std::string safe;
	for (size_t i = 0; i < filename.length(); i++) {
		char c = filename[i];
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == ' ')
			safe += c;
	}
	return safe;
}

std::string FileHandler::constructFilePath(const std::string &uri, const LocationConfig &loc) {
	std::string path = loc.root;
	std::string locationDir = loc.path;

	if (locationDir[0] == '/')
		locationDir = locationDir.substr(1);

	if (uri.find(loc.path) == 0) {
		if (!path.empty() && path[path.length()-1] != '/')
			path += '/';
		path += locationDir;

		std::string remaining = uri.substr(loc.path.length());
		if (!remaining.empty()) {
			if (remaining[0] == '/')
				remaining = remaining.substr(1);
			if (!remaining.empty()) {
				if (path[path.length()-1] != '/')
					path += '/';
				path += remaining;
			}
		}
	}

	return urlDecode(path);
}


bool FileHandler::isValidFilePath(const std::string &path) {
	if (path.find("..") != std::string::npos)	// Check for path traversal
		return false;

	struct stat st;
	if (stat(path.c_str(), &st) != 0)	// Check if the path exists
		return false;

	if (access(path.c_str(), R_OK) != 0)	// Check if the file is accessible
		return false;

	return true;
}
