/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:37:57 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

Request::Request(const Request &other) {
	_method = other._method;
	_path = other._path;
	_queryString = other._queryString;
	_version = other._version;
	_headers = other._headers;
	_body = other._body;
	_config = other._config;
	_isChunked = other._isChunked;
	_tempFilePath = other._tempFilePath;
}

Request &Request::operator=(const Request &other) {
	if (this != &other) {
		_method = other._method;
		_path = other._path;
		_queryString = other._queryString;
		_version = other._version;
		_headers = other._headers;
		_body = other._body;
		_config = other._config;
		_isChunked = other._isChunked;
		_tempFilePath = other._tempFilePath;
	}
	return *this;
}

bool Request::parse(const std::string &rawRequest) {
	size_t firstLineEnd = rawRequest.find("\r\n");
	if (firstLineEnd == std::string::npos)
		return false;

	// Parse request line
	if (!parseRequestLine(rawRequest.substr(0, firstLineEnd)))
		return false;

	// Parse headers
	size_t headerEnd = rawRequest.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return false;

	if (!parseHeaders(rawRequest.substr(firstLineEnd + 2, headerEnd - (firstLineEnd + 2))))
		return false;

	// Check transfer encoding first
	_isChunked = (getHeader("Transfer-Encoding") == "chunked");
	size_t bodyStart = headerEnd + 4; // Start of body after headers

	if (_isChunked) {
		try {
			// Extract and process chunked body
			std::string chunkedBody = rawRequest.substr(bodyStart);
			_body = unchunkData(chunkedBody);
			_headers["Content-Length"] = Utils::numToString(_body.length());
			_headers.erase("Transfer-Encoding");
		} catch (const std::exception &e) {
			return false;
		}
	} else {
		// Handle standard body with Content-Length
		std::string contentLength = getHeader("Content-Length");
		if (!contentLength.empty()) {
			_body = rawRequest.substr(bodyStart);
			_headers["Content-Length"] = Utils::numToString(_body.length());
		}
	}
	parseCookies();
	return true;
}

bool Request::parseRequestLine(const std::string &line) {
	size_t first = line.find(' ');
	size_t last = line.rfind(' ');

	if (first == std::string::npos || last == std::string::npos || first == last)
		return false;

	_method = line.substr(0, first);

	// Extract path and query string
	std::string fullPath = line.substr(first + 1, last - first - 1);
	size_t		queryPos = fullPath.find('?');
	if (queryPos != std::string::npos) {
		_path = fullPath.substr(0, queryPos);
		_queryString = fullPath.substr(queryPos + 1);
	} else {
		_path = fullPath;
		_queryString = "";
	}

	_version = line.substr(last + 1);

	return true;
}

bool Request::parseHeaders(const std::string &headerSection) {
	if (headerSection.empty())
		return false;

	std::istringstream stream(headerSection);
	std::string		   line;
	bool			   hasValidHeaders = false;

	while (std::getline(stream, line)) {
		// Skip empty lines
		if (line.empty() || line == "\r")
			continue;

		// Remove trailing CR if present
		if (line[line.length() - 1] == '\r')
			line = line.substr(0, line.length() - 1);

		// Parse header line
		size_t colonPos = line.find(": ");
		if (colonPos == std::string::npos)
			continue;

		std::string key = trimWhitespace(line.substr(0, colonPos));
		std::string value = trimWhitespace(line.substr(colonPos + 2));

		// Store valid headers
		if (!key.empty()) {
			_headers[key] = value;
			hasValidHeaders = true;
		}
	}

	return hasValidHeaders;
}

std::string Request::trimWhitespace(const std::string &str) {
	size_t first = str.find_first_not_of(" \t");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t");
	return str.substr(first, last - first + 1);
}

// Getters
const std::string &Request::getMethod() const {
	return _method;
}

const std::string &Request::getPath() const {
	return _path;
}

const std::string &Request::getBody() const {
	if (!_tempFilePath.empty())
		const_cast<Request *>(this)->loadBodyFromTempFile();
	return _body;
}

void Request::loadBodyFromTempFile() {
	if (!_tempFilePath.empty()) {
		int fd = open(_tempFilePath.c_str(), O_RDONLY);
		if (fd != -1) {
			char	buffer[8192];
			ssize_t bytes;
			while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
				_body.append(buffer, bytes);
			}
			close(fd);
			unlink(_tempFilePath.c_str());
			_tempFilePath.clear();
		}
	}
}

bool Request::hasHeader(const std::string &name) const {
	return _headers.find(name) != _headers.end();
}

std::string Request::getHeader(const std::string &name) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(name);
	return it != _headers.end() ? it->second : "";
}

void Request::setConfig(const void *config) {
	_config = config;
}

bool Request::isChunked() const {
	return _isChunked;
}

std::string Request::unchunkData(const std::string &chunkedData) {
	// Pre-allocate result buffer to avoid reallocations
	std::string result;
	result.reserve(chunkedData.length()); // Worst case size
	size_t		 pos = 0;
	const size_t len = chunkedData.length();

	while (pos < len) {
		// Find chunk size end
		size_t endOfSize = chunkedData.find("\r\n", pos);
		if (endOfSize == std::string::npos)
			throw std::runtime_error("Invalid chunk format");

		// Parse chunk size more efficiently
		size_t chunkSize = 0;
		for (size_t i = pos; i < endOfSize; ++i) {
			char c = chunkedData[i];
			if (c >= '0' && c <= '9')
				chunkSize = (chunkSize << 4) | (c - '0');
			else if (c >= 'a' && c <= 'f')
				chunkSize = (chunkSize << 4) | (c - 'a' + 10);
			else if (c >= 'A' && c <= 'F')
				chunkSize = (chunkSize << 4) | (c - 'A' + 10);
			else
				break; // Handle optional chunk extensions
		}
		if (chunkSize == 0) // End of chunks
			break;

		size_t dataStart = endOfSize + 2;	 // Skip CRLF after size
		if (dataStart + chunkSize + 2 > len) // +2 for trailing CRLF
			throw std::runtime_error("Incomplete chunk data");

		result.append(chunkedData.data() + dataStart, chunkSize);

		// Move to next chunk
		pos = dataStart + chunkSize + 2; // Skip data and CRLF
	}
	return result;
}

void Request::setTempFilePath(const std::string &path) {
	_tempFilePath = path;
}

void Request::parseCookies() {
	std::string cookieHeader = getHeader("Cookie");
	if (cookieHeader.empty())
		return;

	std::istringstream cookieStream(cookieHeader);
	std::string		   cookie;

	while (std::getline(cookieStream, cookie, ';')) {
		// Trim leading/trailing whitespace
		cookie = trimWhitespace(cookie);

		size_t equalPos = cookie.find('=');
		if (equalPos != std::string::npos) {
			std::string name = cookie.substr(0, equalPos);
			std::string value = cookie.substr(equalPos + 1);
			_cookies[name] = value;
		}
	}
}

std::map<std::string, std::string> Request::getCookies() const {
	return _cookies;
}

void Request::clearBody() {
	std::string().swap(_body);
}
