/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/27 12:10:38 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>

/**
 * @brief Parses raw HTTP request string
 * @param raw Complete HTTP request as string
 * @return true if parsing successful, false otherwise
 */
bool HTTPRequest::parse(const std::string &rawRequest) {
	// Debug output
	std::cout << "Parse called with request size: " << rawRequest.size() << std::endl;

	// First parse request line
	size_t firstLineEnd = rawRequest.find("\r\n");
	if (firstLineEnd == std::string::npos) {
		std::cout << "No request line found" << std::endl;
		return false;
	}

	// Parse first line
	std::string firstLine = rawRequest.substr(0, firstLineEnd);
	if (!parseRequestLine(firstLine)) {
		std::cout << "Failed to parse request line" << std::endl;
		return false;
	}

	size_t headerEnd = rawRequest.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		std::cout << "No header end found" << std::endl;
		return false;
	}

	// Parse headers
	std::string headerSection = rawRequest.substr(firstLineEnd + 2,
												  headerEnd - (firstLineEnd + 2));
	if (!parseHeaders(headerSection)) {
		std::cout << "Failed to parse headers" << std::endl;
		return false;
	}

	// Get Content-Length
	size_t contentLength = 0;
	if (_headers.find("Content-Length") != _headers.end()) {
		contentLength = std::atol(_headers["Content-Length"].c_str());
		std::cout << "Content-Length: " << contentLength << std::endl;
	}

	// Extract body
	if (contentLength > 0) {
		size_t bodyStart = headerEnd + 4;
		if (rawRequest.length() >= bodyStart + contentLength) {
			_body = rawRequest.substr(bodyStart, contentLength);
			std::cout << "Body size: " << _body.length() << std::endl;
			return true;
		} else {
			std::cout << "Incomplete body" << std::endl;
			return false;
		}
	}

	return true;
}

/**
 * @brief Parses first line of HTTP request
 * @param line Request line containing method, path, version
 * @return true if parsing successful, false otherwise
 */
bool HTTPRequest::parseRequestLine(const std::string &line) {
	// Debug
	std::cout << "Parsing request line: " << line << std::endl;

	size_t first = line.find(' ');
	size_t last = line.rfind(' ');

	if (first == std::string::npos || last == std::string::npos || first == last) {
		std::cout << "Invalid request line format" << std::endl;
		return false;
	}

	_method = line.substr(0, first);
	_path = line.substr(first + 1, last - first - 1);
	_version = line.substr(last + 1);

	// Debug
	std::cout << "Method: '" << _method << "'" << std::endl;
	std::cout << "Path: '" << _path << "'" << std::endl;
	std::cout << "Version: '" << _version << "'" << std::endl;

	return true;
}

/**
 * @brief Parses HTTP headers from request
 * @param request Request string starting after request line
 * @return true if parsing successful, false otherwise
 */
bool HTTPRequest::parseHeaders(const std::string &headerSection) {
	std::string line;
	std::istringstream headerStream(headerSection);

	while (std::getline(headerStream, line)) {
		// Skip empty lines
		if (line.empty() || line == "\r")
			continue;

		// Remove trailing \r if present
		if (!line.empty() && line[line.length()-1] == '\r')
			line = line.substr(0, line.length()-1);

		size_t colonPos = line.find(": ");
		if (colonPos == std::string::npos)
			continue;

		std::string key = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 2);
		_headers[key] = value;

		std::cout << "Parsed header: " << key << " = " << value << std::endl;
	}
	return true;
}

/**
 * @brief Extracts body based on Content-Length header
 * @param request Request string starting after headers
 */
void HTTPRequest::parseBody(std::string &request) {
	std::map<std::string, std::string>::const_iterator it = _headers.find("Content-Length");
	if (it != _headers.end()) {
		char *endptr;
		unsigned long length = std::strtoul(it->second.c_str(), &endptr, 10);
		if (*endptr != '\0') // Invalid format
			return ;
		if (length > request.length())	// Check overflow
			length = request.length();
		_body = request.substr(0, length);
	}
}

/**
 * @brief Removes leading/trailing whitespace from string
 * @param str String to trim
 * @return Trimmed string
 */
std::string HTTPRequest::trimWhitespace(const std::string &str) {
	size_t first = str.find_first_not_of(" \t");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t");
	return str.substr(first, last - first + 1);
}

/* Getters */
const std::string &HTTPRequest::getMethod() const { return _method; }
const std::string &HTTPRequest::getPath() const { return _path; }
const std::string &HTTPRequest::getVersion() const { return _version; }
const std::string &HTTPRequest::getBody() const { return _body; }

/**
 * @brief Checks if header exists
 * @param name Header name (case-sensitive)
 * @return true if header exists
 */
bool HTTPRequest::hasHeader(const std::string &name) const {
	return _headers.find(name) != _headers.end();
}

/**
 * @brief Gets header value
 * @param name Header name (case-sensitive)
 * @return Header value or empty string if not found
 */
std::string HTTPRequest::getHeader(const std::string &name) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(name);
	return it != _headers.end() ? it->second : "";
}

const std::map<std::string, std::string> &HTTPRequest::getHeaders() const { return _headers; }
