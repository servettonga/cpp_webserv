/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 17:30:36 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"
#include <sstream>
#include <cstdlib>

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

	// Handle body if Content-Length present
	std::string contentLength = getHeader("Content-Length");
	if (!contentLength.empty()) {
		size_t bodyLength = std::atol(contentLength.c_str());
		size_t bodyStart = headerEnd + 4;

		if (rawRequest.length() < bodyStart + bodyLength)
			return false;

		_body = rawRequest.substr(bodyStart, bodyLength);
	}

	return true;
}

bool HTTPRequest::parseRequestLine(const std::string &line) {
	size_t first = line.find(' ');
	size_t last = line.rfind(' ');

	if (first == std::string::npos || last == std::string::npos || first == last)
		return false;

	_method = line.substr(0, first);
	_path = line.substr(first + 1, last - first - 1);
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
