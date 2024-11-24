/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/23 22:38:31 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"
#include <cstdlib>

/**
 * @brief Parses raw HTTP request string
 * @param raw Complete HTTP request as string
 * @return true if parsing successful, false otherwise
 */
bool HTTPRequest::parse(const std::string &raw) {
	std::string request = raw;
	size_t pos = request.find("\r\n");
	if (pos == std::string::npos)
		return false;

	if (!parseRequestLine(request.substr(0, pos)))
		return false;

	request = request.substr(pos + 2);
	if (!parseHeaders(request))
		return false;

	parseBody(request);
	return true;
}

/**
 * @brief Parses first line of HTTP request
 * @param line Request line containing method, path, version
 * @return true if parsing successful, false otherwise
 */
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

/**
 * @brief Parses HTTP headers from request
 * @param request Request string starting after request line
 * @return true if parsing successful, false otherwise
 */
bool HTTPRequest::parseHeaders(std::string &request) {
	while (true) {
		size_t pos = request.find("\r\n");
		if (pos == std::string::npos)
			return false;

		std::string line = request.substr(0, pos);
		if (line.empty()) {
			request = request.substr(pos + 2);
			return true;
		}

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			return false;

		std::string key = trimWhitespace(line.substr(0, colon));
		std::string value = trimWhitespace(line.substr(colon + 1));

		if (!key.empty())
			_headers[key] = value;

		request = request.substr(pos + 2);
	}
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
