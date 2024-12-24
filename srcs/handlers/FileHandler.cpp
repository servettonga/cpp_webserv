/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:09:27 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/12 23:27:47 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FileHandler.hpp"

Response FileHandler::serveFile(const std::string &path, const std::string &urlPath) {
	if (!isValidFilePath(path))
		return Response(403, "Forbidden");

	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		return Response(404, "Not Found");

	struct stat st;
	if (fstat(fd, &st) < 0) {
		close(fd);
		return Response(500, "Internal Server Error");
	}

	Response response(200);
	response.addHeader("Content-Type", getType(urlPath));
	response.addHeader("Content-Length", Utils::numToString(st.st_size));
	response.setFileDescriptor(fd);

	return response;
}

Response FileHandler::handleFileUpload(const Request &request, const LocationConfig &loc) {
	std::string boundary = extractBoundary(request.getHeader("Content-Type"));
	if (boundary.empty())
		return Response(400, "Bad Request - Invalid Content-Type");

	// Determine the upload path
	std::string uploadPath;
	if (!loc.root.empty()) {
		uploadPath = loc.root;
		if (uploadPath[uploadPath.length()-1] != '/')
			uploadPath += '/';
		if (!loc.path.empty() && loc.path != "/") {
			std::string pathComponent = loc.path;
			if (pathComponent[0] == '/')
				pathComponent = pathComponent.substr(1);
			uploadPath += pathComponent;
		}
	} else {
		uploadPath = "www/upload"; // Default fallback
	}

	// Create directory if it doesn't exist
	struct stat st;
	if (stat(uploadPath.c_str(), &st) != 0) {
		std::string mkdirCmd = "mkdir -p " + uploadPath;
		if (system(mkdirCmd.c_str()) != 0)
			return Response(500, "Internal Server Error - Cannot create upload directory");
		// Set directory permissions
		if (chmod(uploadPath.c_str(), 0755) != 0)
			return Response(500, "Internal Server Error - Cannot set directory permissions");
	}

	// Parse multipart form data
	FileData fileData = parseMultipartData(request.getBody(), boundary);
	if (!fileData.isValid)
		return Response(400, "Bad Request - Invalid file data");

	// Construct final file path
	std::string filepath = uploadPath + "/" + fileData.filename;

	// Save file
	if (!saveUploadedFile(filepath, fileData.content))
		return Response(500, "Internal Server Error - File save failed");

	// Return success response
	Response response(201);
	response.addHeader("Content-Type", "text/plain");
	response.setBody("File uploaded successfully");
	return response;
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

bool FileHandler::saveUploadedFile(const std::string &filepath, const std::string &content) {
	const size_t CHUNK_SIZE = 65536;
	int fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) return false;

	size_t remaining = content.length();
	size_t offset = 0;

	while (remaining > 0) {
		size_t chunk = std::min(remaining, CHUNK_SIZE);
		ssize_t written = write(fd, content.c_str() + offset, chunk);
		if (written < 0) {
			close(fd);
			return false;
		}
		offset += written;
		remaining -= written;
	}

	close(fd);
	return true;
}

Response FileHandler::handleFileDelete(const Request &request, const LocationConfig &loc) {
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

std::string FileHandler::constructFilePath(const std::string &uri, const LocationConfig &location) {
	std::string decodedUri = urlDecode(uri);
	std::string path = location.root;
	// Remove trailing slash from root if present
	if (!path.empty() && path[path.length()-1] == '/')
		path = path.substr(0, path.length()-1);
	// For non-root locations in www directory
	if (location.path != "/" && !location.path.empty() && location.root == "www") {
		std::string locationPath = location.path;
		if (locationPath[0] == '/')
			locationPath = locationPath.substr(1);
		if (!path.empty() && path[path.length()-1] != '/')
			path += '/';
		path += locationPath;
	}
	// Process the relative path
	std::string relativePath;
	if (decodedUri.find(location.path) == 0) {
		if (decodedUri != location.path) {
			relativePath = decodedUri.substr(location.path.length());
			if (!relativePath.empty() && relativePath[0] == '/')
				relativePath = relativePath.substr(1);
		}
	}
	// Combine paths
	if (!relativePath.empty()) {
		if (!path.empty() && path[path.length()-1] != '/')
			path += '/';
		path += relativePath;
	}
	return path;
}

bool FileHandler::isValidFilePath(const std::string &path) {
	if (path.find("..") != std::string::npos)
		return false;

	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;

	// Allow execute permission for .bla files
	if (path.find(".bla") != std::string::npos) {
		return (access(path.c_str(), R_OK | X_OK) == 0);
	}

	return (access(path.c_str(), R_OK) == 0);
}

std::string FileHandler::getType(const std::string &path) {
	if (path == "/" || path.empty())
		return "text/html";

	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos)
		return "text/html";

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

	return "text/plain";
}
