/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/06 18:36:17 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest() {
	/*
		Constructor(rawRequest):
		1. Initialize empty request
		2. Parse raw request if provided
	*/
}

HTTPRequest::HTTPRequest(const std::string &rawRequest) {
	(void)rawRequest;
}

HTTPRequest::~HTTPRequest() {

}

const std::string &HTTPRequest::getMethod() const {	return _method; }

const std::string &HTTPRequest::getURI() const { return _uri; }

const std::string &HTTPRequest::getVersion() const { return _version; }

const std::map<std::string, std::string> &HTTPRequest::getHeaders() const { return _headers; }

const std::string &HTTPRequest::getBody() const { return _body; }

const std::string &HTTPRequest::getBoundary() const { return _boundary; }

void HTTPRequest::parseRequest(const std::string &rawRequest) {
	/*
		parseRequest(rawRequest):
		1. Find headers/body separator
		2. Extract and parse request line
		3. Parse headers section
		4. Parse body if present
		5. Handle special content types
	*/
	(void)rawRequest;
}

void HTTPRequest::parseHeaders(const std::string &headerSection) {
	/*
		parseHeaders(headerSection):
		1. Split into individual headers
		2. FOR each header:
		   - Split into name and value
		   - Normalize header name
		   - Trim whitespace
		   - Store in headers map
		3. Process special headers:
		   - Content-Type (extract boundary)
		   - Content-Length
		   - Transfer-Encoding
	*/
	(void)headerSection;
}

void HTTPRequest::parseBody(const std::string &bodySection) {
	/*
		parseBody(bodySection):
		1. Check Content-Type
		2. IF multipart/form-data:
		   - Parse multipart data
		3. ELSE IF application/x-www-form-urlencoded:
		   - Parse URL encoded data
		4. ELSE IF chunked encoding:
		   - Decode chunked data
		5. Store raw body
	*/
	(void)bodySection;
}

std::vector<HTTPRequest::FormDataPart> HTTPRequest::parseMultipartFormData() {
	/*
		struct FormDataPart:
		A simple structure to hold multipart form data parts
		Members:
		- headers: map of headers specific to this part
		- content: raw content of the part
	*/

	/*
		parseMultipartFormData():
		1. Get boundary from Content-Type header:
		   - Extract from "multipart/form-data; boundary=..."
		   - Add "--" prefix to boundary

		2. Initialize vector for parts

		3. Find the first boundary in body
		   IF not found:
			 Return empty vector

		4. WHILE the next boundary exists:
		   - Extract content between boundaries
		   - Create new FormDataPart

		   - Parse part headers:
			 WHILE header line exists:
			   IF empty line:
				 Break (headers end)
			   Parse "Header: Value"
			   Add to part.headers

		   - Store remaining content in part.content
		   - Add part to vector

		5. Return vector of parts

		Note: Handle special cases:
		- Last boundary with "--" suffix
		- Empty parts
		- Missing headers
		- Malformed boundaries
	*/
	return std::vector<FormDataPart>();
}

void HTTPRequest::parseRequestLine(const std::string &requestLine) {
	/*
		parseRequestLine(requestLine):
		1. Split line into method, URI, version
		2. Validate method
		3. Decode URI
		4. Extract query string
		5. Normalize path
		6. Validate the HTTP version
	*/
	(void)requestLine;
}

void HTTPRequest::parseFormDataPart(const std::string &partData, HTTPRequest::FormDataPart &part) {
	/*
		parseFormDataPart(partData, part):
		1. Split headers and content
		2. Parse part headers
		3. Store content
	*/
	(void)partData;
	(void)part;
}

bool HTTPRequest::isValidMethod(const std::string &method) {
	/*
		isValidMethod(method):
		1. Check against allowed methods:
		   - GET
		   - POST
		   - DELETE
		   - HEAD
		   - PUT (if supported)
		2. Return validation result
	*/
	(void)method;
	return false;
}

void HTTPRequest::extractQueryString() {
	/*
		extractQueryString():
		1. Find '?' in URI
		2. Split into path and query
		3. Store query string
		4. Update URI to just path
	*/
}

void HTTPRequest::normalizePath() {
	/*
		normalizePath():
		1. Remove dot segments
		2. Resolve relative paths
		3. Remove duplicate slashes
		4. Decode percent-encoded characters
	*/
}

std::string HTTPRequest::decodeURIComponent(const std::string &encoded) {
	/*
		decodeURIComponent(encoded):
		1. Find percent-encoded sequences
		2. Convert to characters
		3. Handle UTF-8 encoding
		4. Return decoded string
	*/
	(void)encoded;
	return std::string();
}
