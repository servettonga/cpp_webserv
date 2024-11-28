/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/28 14:12:08 by jdepka           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest() : _method(""), _uri(""), _version(""), _body(""), _boundary("") {
	/*
		Constructor(rawRequest):
		1. Initialize empty request
		2. Parse raw request if provided
	*/
}

HTTPRequest::HTTPRequest(const std::string &rawRequest) {
	parseRequest(rawRequest);
}

HTTPRequest::~HTTPRequest() {}

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
    const size_t MAX_REQUEST_SIZE = 1048576;
    if (rawRequest.size() > MAX_REQUEST_SIZE) {
        throw std::overflow_error("Request size exceeds maximum allowed limit");
    }
    size_t separatorPos = rawRequest.find("\r\n\r\n");
    if (separatorPos == std::string::npos) {
        throw std::invalid_argument("Malformed request: missing header/body separator");
    }
    std::string requestLineAndHeaders = rawRequest.substr(0, separatorPos);
    std::string body = rawRequest.substr(separatorPos + 4);
    size_t lineEndPos = requestLineAndHeaders.find("\r\n");
    if (lineEndPos == std::string::npos) {
        throw std::invalid_argument("Malformed request: missing request line");
    }
    std::string requestLine = requestLineAndHeaders.substr(0, lineEndPos);
    try {
        parseRequestLine(requestLine);
    } catch (const std::exception &e) {
        throw std::invalid_argument("Error parsing request line: " + std::string(e.what()));
    }
    std::string headers = requestLineAndHeaders.substr(lineEndPos + 2);
    try {
        parseHeaders(headers);
    } catch (const std::exception &e) {
        throw std::invalid_argument("Error parsing headers: " + std::string(e.what()));
    }
    if (!body.empty()) {
        try {
            parseBody(body);
        } catch (const std::exception &e) {
            throw std::runtime_error("Error parsing body: " + std::string(e.what()));
        }
    }
}

// Help function to normalize header name
std::string normalizeHeaderName(const std::string& input) {
    std::string result;
    bool newWord = true; 
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (c == '-') {
            result += c;
            newWord = true;
            continue;
        }
        if (newWord) {
            result += std::toupper(c);
            newWord = false;
        } else {
            result += std::tolower(c);
        }
    }
    return result;
}

// Helper function for trimming
static void trim(std::string &str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    if (start == std::string::npos || end == std::string::npos) {
        str.clear();
    } else {
        str = str.substr(start, end - start + 1);
    }
}

// Helper function for validating header name
static bool isValidHeaderName(const std::string &name) {
    for (size_t i = 0; i < name.size(); ++i) {
        if (!isalnum(name[i]) && name[i] != '-' && name[i] != '_') {
            return false;
        }
    }
    return true;
}

// Helper function for validating header value
static bool isValidHeaderValue(const std::string &value) {
    if (value.empty()) {
        return false;
    }
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] != 9) {
            if (value[i] < 32 || value[i] > 126) {
                return false;
            }
        }
    }
    return true;
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
	const size_t MAX_HEADER_SIZE = 8192;
    if (headerSection.size() > MAX_HEADER_SIZE) {
        throw std::overflow_error("Header section exceeds maximum allowed size");
    }
    std::istringstream stream(headerSection);
    std::string line;
    bool isHeaderEnd = false;
    while (std::getline(stream, line)) {
        trim(line);
        if (line.empty()) continue;
        size_t colonPos = line.find(":");
        if (colonPos == std::string::npos) {
            throw std::invalid_argument("Malformed header line: missing colon");
        }
        std::string headerName = line.substr(0, colonPos);
        std::string headerValue = line.substr(colonPos + 1);
        trim(headerName);
        trim(headerValue);
        if (!isValidHeaderName(headerName)) {
            throw std::invalid_argument("Invalid characters in header name");
        }
        if (!isValidHeaderValue(headerValue)) {
            throw std::invalid_argument("Invalid characters in header value");
        }
        headerName = normalizeHeaderName(headerName);
        if (_headers.find(headerName) != _headers.end()) {
            if (headerName == "Set-Cookie") {
                _headers[headerName] += ", " + headerValue;
            } else {
                _headers[headerName] = headerValue;
            }
        } else {
            _headers[headerName] = headerValue;
        }
        if (headerName == "Content-Type") {
            size_t boundaryPos = headerValue.find("boundary=");
            if (boundaryPos != std::string::npos) {
                _boundary = headerValue.substr(boundaryPos + 9);
                trim(_boundary);
            }
        }
    }
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
    _body = bodySection;
    auto it = _headers.find("ContentType");
    if (it == _headers.end()) {
        return; 
    }
    const std::string &contentType = it->second;
    if (contentType.find("multipart/form-data") != std::string::npos) {
        if (_boundary.empty()) {
            throw std::runtime_error("Boundary not defined for multipart/form-data");
        }
        try {
            parseMultipartFormData();
        } catch (const std::exception &e) {
            throw std::runtime_error("Error parsing multipart/form-data: " + std::string(e.what()));
        }
        return;
    }
    if (contentType == "application/x-www-form-urlencoded") {
        std::istringstream stream(bodySection);
        std::string pair;
        while (std::getline(stream, pair, '&')) {
            size_t pos = pair.find('=');
            if (pos == std::string::npos) {
                throw std::invalid_argument("Malformed URL-encoded form data");
            }
            std::string key = decodeURIComponent(pair.substr(0, pos));
            std::string value = decodeURIComponent(pair.substr(pos + 1));
            _headers[key] = value;
        }
        return;
    }
    if (_headers.find("TransferEncoding") != _headers.end() &&
        _headers["TransferEncoding"] == "chunked") {
        std::string decodedBody;
        size_t pos = 0;
        while (pos < bodySection.size()) {
            size_t chunkEnd = bodySection.find("\r\n", pos);
            if (chunkEnd == std::string::npos) {
                throw std::runtime_error("Malformed chunked body: missing chunk size");
            }
            std::string chunkSizeStr = bodySection.substr(pos, chunkEnd - pos);
            size_t chunkSize = 0;
            try {
                chunkSize = std::stoul(chunkSizeStr, nullptr, 16);
            } catch (...) {
                throw std::runtime_error("Invalid chunk size");
            }
            if (chunkSize == 0) {
                break;
            }
            pos = chunkEnd + 2;
            if (pos + chunkSize > bodySection.size()) {
                throw std::runtime_error("Malformed chunked body: chunk exceeds body size");
            }
            decodedBody += bodySection.substr(pos, chunkSize);
            pos += chunkSize + 2;
        }
        _body = decodedBody;
        return;
    }
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
    std::vector<FormDataPart> parts;
    if (_boundary.empty()) {
        throw std::runtime_error("Boundary not found in Content-Type header");
    }
    const std::string boundaryWithPrefix = _boundary;
    const std::string boundaryEnd = boundaryWithPrefix + "--";
    size_t pos = 0;
    size_t nextBoundaryPos;
    const size_t MAX_BODY_SIZE = 1048576;
    const size_t MAX_PARTS = 100;
    if (_body.size() > MAX_BODY_SIZE) {
        throw std::overflow_error("Body size exceeds maximum allowed limit");
    }
    while ((nextBoundaryPos = _body.find(boundaryWithPrefix, pos)) != std::string::npos) {
        size_t partStart = pos + boundaryWithPrefix.length();
        if (_body.substr(partStart, 2) == "--") {
            break;
        }
        size_t partEnd = _body.find(boundaryWithPrefix, partStart);
        if (partEnd == std::string::npos) {
            partEnd = _body.find(boundaryEnd, partStart);
        }
        if (partEnd == std::string::npos) {
            throw std::runtime_error("Malformed multipart form data: missing boundary");
        }
        std::string partData = _body.substr(partStart, partEnd - partStart);
        FormDataPart part;
        try {
            parseFormDataPart(partData, part);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error parsing multipart part: " + std::string(e.what()));
        }
        parts.push_back(part);
        if (parts.size() > MAX_PARTS) {
            throw std::overflow_error("Exceeded maximum number of form-data parts");
        }
        pos = partEnd;
    }
    return parts;
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
	std::istringstream stream(requestLine);
    std::string method, uri, version;
    if (!(stream >> method >> uri >> version)) {
        throw std::invalid_argument("Malformed request line");
    }
    if (!isValidMethod(method)) {
        throw std::invalid_argument("Invalid HTTP method");
    }
    _uri = decodeURIComponent(uri);
    normalizePath();
    if (_uri.empty() || _uri[0] != '/') {
        throw std::invalid_argument("Invalid URI path");
    }
    extractQueryString();
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        throw std::invalid_argument("Invalid HTTP version");
    }
    _method = method;
    _version = version;
}

void HTTPRequest::parseFormDataPart(const std::string &partData, HTTPRequest::FormDataPart &part) {
	/*
		parseFormDataPart(partData, part):
		1. Split headers and content
		2. Parse part headers
		3. Store content
	*/
    const size_t MAX_PART_SIZE = 1024 * 1024;
    if (partData.size() > MAX_PART_SIZE) {
        throw std::overflow_error("Part data size exceeds allowed limit");
    }
    size_t headerEnd = partData.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        throw std::invalid_argument("Malformed part: missing headers or content");
    }
    std::string headers = partData.substr(0, headerEnd);
    try {
        parseHeaders(headers);
    } catch (const std::exception &e) {
        throw std::invalid_argument("Malformed part headers: " + std::string(e.what()));
    }
    size_t contentStart = headerEnd + 4;
    part.content = partData.substr(contentStart);

    if (part.content.size() > MAX_PART_SIZE) {
        throw std::overflow_error("Part content size exceeds allowed limit");
    }
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
	return method == "GET" || method == "POST" || method == "DELETE" || method == "HEAD" || method == "PUT";
}

void HTTPRequest::extractQueryString() {
	/*
		extractQueryString():
		1. Find '?' in URI
		2. Split into path and query
		3. Store query string
		4. Update URI to just path
	*/
	size_t pos = _uri.find('?');
    if (pos != std::string::npos) {
        _uri = _uri.substr(0, pos);
    }
}

void HTTPRequest::normalizePath() {
	/*
		normalizePath():
		1. Remove dot segments
		2. Resolve relative paths
		3. Remove duplicate slashes
		4. Decode percent-encoded characters
	*/
    std::istringstream pathStream(_uri);
    std::string segment;
    std::vector<std::string> pathSegments;
    while (std::getline(pathStream, segment, '/')) {
        if (segment == "." || segment.empty()) {
            continue;
        }
        if (segment == "..") {
            if (!pathSegments.empty()) {
                pathSegments.pop_back();
            }
        } else {
            pathSegments.push_back(segment);
        }
    }
    std::ostringstream normalizedPath;
    for (size_t i = 0; i < pathSegments.size(); ++i) {
        if (i != 0) {
            normalizedPath << "/";
        }
        normalizedPath << pathSegments[i];
    }
    _uri = "/" + normalizedPath.str();
    std::string::size_type pos = 0;
    while ((pos = _uri.find("//", pos)) != std::string::npos) {
        _uri.replace(pos, 2, "/");
    }
    std::string decodedPath;
    for (size_t i = 0; i < _uri.size(); ++i) {
        if (_uri[i] == '%' && i + 2 < _uri.size()) {
            char decodedChar;
            std::stringstream ss;
            ss << std::hex << _uri.substr(i + 1, 2);
            ss >> decodedChar;
            decodedPath += decodedChar;
            i += 2;
        } else {
            decodedPath += _uri[i];
        }
    }
    _uri = decodedPath;
}

std::string HTTPRequest::decodeURIComponent(const std::string &encoded) {
	std::string decoded;
	size_t i = 0;
	while (i < encoded.size()) {
		if (encoded[i] == '%') {
			// Ensure there's enough room for a valid %XX sequence
			if (i + 2 < encoded.size()) {
				// Extract the next two characters and convert them from hex
				char hex[3] = { encoded[i + 1], encoded[i + 2], '\0' };
				// Convert hex to decimal and append to the result
				int value;
				std::istringstream(hex) >> std::hex >> value;
				decoded += static_cast<char>(value);
				i += 3; // Move past the %XX
			} else {
				throw std::invalid_argument("Invalid URL encoding");
			}
		} else if (encoded[i] == '+') {
			// '+' is treated as space
			decoded += ' ';
			i++;
		} else {
			// Regular character, just append it
			decoded += encoded[i];
			i++;
		}
	}
	return decoded;
}