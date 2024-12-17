/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/17 11:50:33 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"
#include "../utils/Utils.hpp"
#include <cstring>
#include <sstream>
#include <cstdlib>

HTTPRequest::HTTPRequest(const HTTPRequest &other) {
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

HTTPRequest &HTTPRequest::operator=(const HTTPRequest &other) {
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

bool HTTPRequest::parse(const std::string &rawRequest) {
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

	if (!parseHeaders(rawRequest.substr(firstLineEnd + 2,
										headerEnd - (firstLineEnd + 2))))
		return false;

	// Check transfer encoding first
	_isChunked = (getHeader("Transfer-Encoding") == "chunked");
	size_t bodyStart = headerEnd + 4;  // Start of body after headers

	if (_isChunked) {
		try {
			// Extract and process chunked body
			std::string chunkedBody = rawRequest.substr(bodyStart);
			_body = unchunkData(chunkedBody);

			// Update Content-Length header after unchunking
			_headers["Content-Length"] = Utils::numToString(_body.length());
			// Remove Transfer-Encoding header as we've processed it
			_headers.erase("Transfer-Encoding");
		} catch (const std::exception& e) {
			return false;
		}
	} else {
		// Handle standard body with Content-Length
		std::string contentLength = getHeader("Content-Length");
		if (!contentLength.empty()) {
			_body = rawRequest.substr(bodyStart);
			// Set Content-Length to actual body size
			_headers["Content-Length"] = Utils::numToString(_body.length());
		}
	}

	return true;
}

bool HTTPRequest::parseRequestLine(const std::string &line) {
	size_t first = line.find(' ');
	size_t last = line.rfind(' ');

	if (first == std::string::npos || last == std::string::npos || first == last)
		return false;

	_method = line.substr(0, first);

	// Extract path and query string
	std::string fullPath = line.substr(first + 1, last - first - 1);
	size_t queryPos = fullPath.find('?');
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

bool HTTPRequest::parseHeaders(const std::string &headerSection) {
	if (headerSection.empty())
		return false;

	std::istringstream stream(headerSection);
	std::string line;
	bool hasValidHeaders = false;

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

std::string HTTPRequest::trimWhitespace(const std::string &str) {
	size_t first = str.find_first_not_of(" \t");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t");
	return str.substr(first, last - first + 1);
}

// Getters
const std::string &HTTPRequest::getMethod() const { return _method; }
const std::string &HTTPRequest::getPath() const { return _path; }
const std::string &HTTPRequest::getVersion() const { return _version; }
const std::string &HTTPRequest::getBody() const { return _body; }
const std::map<std::string, std::string> &HTTPRequest::getHeaders() const { return _headers; }

bool HTTPRequest::hasHeader(const std::string &name) const {
	return _headers.find(name) != _headers.end();
}

std::string HTTPRequest::getHeader(const std::string &name) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(name);
	return it != _headers.end() ? it->second : "";
}

void HTTPRequest::setConfig(const void *config) { _config = config; }

const void *HTTPRequest::getConfig() const { return _config; }

bool HTTPRequest::isChunked() const { return _isChunked; }

std::string HTTPRequest::unchunkData(const std::string& chunkedData) {
	// Pre-allocate result buffer to avoid reallocations
	std::string result;
	result.reserve(chunkedData.length());  // Worst case size
	size_t pos = 0;
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
				break;  // Handle optional chunk extensions
		}

		if (chunkSize == 0) // End of chunks
			break;

		// Calculate positions
		size_t dataStart = endOfSize + 2;  // Skip CRLF after size
		if (dataStart + chunkSize + 2 > len) // +2 for trailing CRLF
			throw std::runtime_error("Incomplete chunk data");

		// Append chunk data directly
		result.append(chunkedData.data() + dataStart, chunkSize);

		// Move to next chunk
		pos = dataStart + chunkSize + 2;  // Skip data and CRLF
	}

	return result;
}

void HTTPRequest::setTempFilePath(const std::string &path) { _tempFilePath = path; }

const std::string &HTTPRequest::getTempFilePath() const { return _tempFilePath; }

std::string HTTPRequest::getQueryString() const {
	return _queryString;
}
