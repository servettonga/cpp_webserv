/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 22:41:07 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

HTTPRequest::HTTPRequest(const std::string& rawRequest) {
	parseRequest(rawRequest);
}

HTTPRequest::~HTTPRequest() {
	// Destructor implementation (if needed)
}

void HTTPRequest::parseRequest(const std::string& rawRequest) {
	// Split the raw request into header and body sections
	size_t headerEndPos = rawRequest.find("\r\n\r\n");
	if (headerEndPos == std::string::npos) {
		// Handle error: Invalid HTTP request format
		return;
	}

	std::string headerSection = rawRequest.substr(0, headerEndPos);
	std::string bodySection = rawRequest.substr(headerEndPos + 4);

	// Parse the request line and headers
	parseHeaders(headerSection);

	// Set the body
	_body = bodySection;

	// Extract boundary for multipart/form-data if necessary
	if (_headers.find("Content-Type") != _headers.end()) {
		std::string contentType = _headers["Content-Type"];
		if (contentType.find("multipart/form-data") != std::string::npos) {
			size_t boundaryPos = contentType.find("boundary=");
			if (boundaryPos != std::string::npos) {
				_boundary = "--" + contentType.substr(boundaryPos + 9);
			}
		}
	}
}

void HTTPRequest::parseHeaders(const std::string& headerSection) {
	std::istringstream headerStream(headerSection);
	std::string line;

	// Parse request line
	if (std::getline(headerStream, line)) {
		parseRequestLine(line);
	} else {
		// Handle error: Invalid request line
		return;
	}

	// Parse headers
	while (std::getline(headerStream, line) && !line.empty()) {
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line.erase(line.length() - 1);
		}
		size_t delimiterPos = line.find(':');
		if (delimiterPos != std::string::npos) {
			std::string headerName = line.substr(0, delimiterPos);
			std::string headerValue = line.substr(delimiterPos + 1);

			// Trim whitespace from header name and value
			headerName.erase(headerName.find_last_not_of(" \t") + 1);
			headerValue.erase(0, headerValue.find_first_not_of(" \t"));

			_headers[headerName] = headerValue;
		}
	}
}

void HTTPRequest::parseRequestLine(const std::string& requestLine) {
	std::istringstream lineStream(requestLine);
	lineStream >> _method >> _uri >> _version;
}

void HTTPRequest::parseBody(const std::string &bodySection) {
	// For future use if needed
}

const std::string& HTTPRequest::getMethod() const {
	return _method;
}

const std::string& HTTPRequest::getURI() const {
	return _uri;
}

const std::string& HTTPRequest::getVersion() const {
	return _version;
}

const std::map<std::string, std::string>& HTTPRequest::getHeaders() const {
	return _headers;
}

const std::string& HTTPRequest::getBody() const {
	return _body;
}

const std::string& HTTPRequest::getBoundary() const {
	return _boundary;
}

std::vector<HTTPRequest::FormDataPart> HTTPRequest::parseMultipartFormData() {
	std::vector<FormDataPart> parts;
	if (_boundary.empty()) {
		// No boundary found; not a multipart/form-data request
		return parts;
	}

	size_t pos = 0;
	std::string delimiter = _boundary + "\r\n";
	std::string endBoundary = _boundary + "--";

	while (true) {
		size_t start = _body.find(_boundary, pos);
		if (start == std::string::npos || _body.compare(start, endBoundary.length(), endBoundary) == 0) {
			break; // Reached the end of the parts
		}

		start += _boundary.length() + 2; // Move past boundary and CRLF
		size_t end = _body.find(_boundary, start);
		if (end == std::string::npos) {
			break; // No more parts
		}

		std::string partData = _body.substr(start, end - start - 2); // Exclude trailing CRLF
		FormDataPart part;
		parseFormDataPart(partData, part);
		parts.push_back(part);

		pos = end;
	}

	return parts;
}

void HTTPRequest::parseFormDataPart(const std::string& partData, FormDataPart& part) {
	size_t headerEnd = partData.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		// Handle error: Invalid part format
		return;
	}

	std::string headersStr = partData.substr(0, headerEnd);
	std::string content = partData.substr(headerEnd + 4);

	// Parse headers
	std::istringstream headerStream(headersStr);
	std::string line;
	while (std::getline(headerStream, line) && !line.empty()) {
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line.erase(line.length() - 1);
		}
		size_t delimiterPos = line.find(':');
		if (delimiterPos != std::string::npos) {
			std::string headerName = line.substr(0, delimiterPos);
			std::string headerValue = line.substr(delimiterPos + 1);

			// Trim whitespace
			headerName.erase(headerName.find_last_not_of(" \t") + 1);
			headerValue.erase(0, headerValue.find_first_not_of(" \t"));

			part.headers[headerName] = headerValue;
		}
	}

	part.content = content;
}
