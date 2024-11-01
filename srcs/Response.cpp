/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 23:00:44 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include <sstream>

Response::Response() : _statusCode(200) {
	// Initialize with default headers
	// HTTP/1.1 requires certain headers by default
}

Response::Response(const Response &other) : _statusCode() {
	// Deep copy implementation for:
	// - statusCode
	// - headers
	// - body
}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		// Deep copy implementation
	}
	return *this;
}

Response::~Response() {
	// Clean up if needed
}

void Response::setStatusCode(int code) {
	// Set status code
	// Consider validating the code range (100-599)
}

void Response::setHeader(const std::string& key, const std::string& value) {
	// Add or update header
	// Consider case-sensitivity handling
}

void Response::setBody(const std::string& body) {
	// Set body content
	// Should automatically update Content-Length header
}

// Private helper methods
std::string Response::getReasonPhrase() const {
	// Return standard reason phrase for current status code
}

void Response::updateContentLength() {
	// Update Content-Length header based on body size
}

std::string Response::toString() const {
	std::stringstream response;

	// Add status line with protocol version, status code, and reason phrase
	response << "HTTP/1.1 " << _statusCode << " " << getReasonPhrase() << "\r\n";

	// Add all headers
	std::map<std::string, std::string>::const_iterator it;
	for (it = _headers.begin(); it != _headers.end(); ++it) {
		response << it->first << ": " << it->second << "\r\n";
	}

	// Add empty line to separate headers from body
	response << "\r\n";

	// Add body if present
	if (!_body.empty()) {
		response << _body;
	}

	return response.str();
}
