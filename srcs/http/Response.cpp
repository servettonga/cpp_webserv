/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/27 17:11:48 by jdepka           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

// Helper function to get current date in HTTP format
static std::string getCurrentDate() {
    time_t now = time(nullptr);
    tm *gmt = gmtime(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buf);
}

Response::Response() : _statusCode(200) {
	/*
		Constructor:
		1. Set default status code (200)
		2. Initialize empty headers map
		3. Set default headers:
		   - Date
		   - Server
		   - Connection
	*/
	setDefaultHeaders();
}

Response::Response(int statusCode) {
	/*
		Response(statusCode):
		1. Validate status code
		2. Set status code and text
		3. Initialize empty headers map
		4. Set default headers:
		   - Date: current UTC time
		   - Server: server name
		   - Connection: keep-alive
		Note: Reuse logic from default constructor
	*/
	setStatusCode(statusCode);
    setDefaultHeaders();
}

Response::~Response() {}

void Response::setStatusCode(int code) {
	/*
		setStatusCode(code):
		1. Validate status code
		2. Store status code
		3. Update status text - not needed since status text is based on status code
	*/
	if (!isValidStatusCode(code)) {
        throw std::invalid_argument("Invalid status code");
    }
    _statusCode = code;
}

int Response::getStatusCode() const { return _statusCode; }

// Helper funtion to normalize header name
static std::string normalizeHeaderName(const std::string &name) {
    std::string normalized;
    bool capitalizeNext = true;
    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];
        if (c == '-') {
            normalized += c;
            capitalizeNext = true;
        } else if (capitalizeNext) {
            normalized += static_cast<char>(std::toupper(c));
            capitalizeNext = false;
        } else {
            normalized += static_cast<char>(std::tolower(c));
        }
    }
    return normalized;
}

// Helper function to validade Content-Length value
static bool validateContentLength(const std::string &value) {
	for (char c : value) {
		if (!isdigit(c)) {
			return false;
		}
	}
	return true;
}

void Response::addHeader(const std::string &name, const std::string &value) {
	/*
		addHeader(name, value):
		1. Normalize header name
		2. Validate header value
		3. Store in the headers map
		4. Handle special cases:
		   - Content-Length
		   - Content-Type
		   - Transfer-Encoding
	*/
	std::string normalizedName = normalizeHeaderName(name);
	if (normalizedName == "Content-Length") {
		if (!validateContentLength(value)) {
			throw std::invalid_argument("Invalid Content-Length value: must be a number.");
		}
		
	} else if (normalizedName == "Content-Type") {
		if (!(value.find('/') != std::string::npos)) {
			throw std::invalid_argument("Invalid Content-Type value: must be a valid MIME type.");
		}
	} else if (normalizedName == "Transfer-Encoding") {
		if (value != "chunked") {
			throw std::invalid_argument("Invalid Transfer-Encoding value: only 'chunked' is supported.");
		}
	}
	_headers[normalizedName] = value;
}

void Response::removeHeader(const std::string &name) {
	/*
		removeHeader(name):
		1. Normalize header name (case-insensitive)
		2. IF header exists:
		   - Remove from headers map
		   - IF Content-Length or Content-Type:
			 Update internal state accordingly
	*/
	std::string normalizedName = normalizeHeaderName(name);
		if (hasHeader(normalizedName)) {
		_headers.erase(getHeader(normalizedName));
		if (normalizedName == "Content-Length") {
			setBody("");
		}
	}
}

bool Response::hasHeader(const std::string &name) const {
	/*
		hasHeader(name):
		1. Normalize header name
		2. Return true if header exists in map
		3. Return false otherwise
		Note: Case-insensitive comparison
	*/
	std::string normalizedName = normalizeHeaderName(name);
    return _headers.find(normalizedName) != _headers.end();
}

std::string Response::getHeader(const std::string &name) const {
	/*
		getHeader(name):
		1. Normalize header name
		2. IF header exists:
		   Return header value
		3. ELSE:
		   Return empty string
		Note: Case-insensitive lookup
	*/
	std::string normalizedName = normalizeHeaderName(name);
    std::map<std::string, std::string>::const_iterator it = _headers.find(normalizedName);
    return (it != _headers.end()) ? it->second : "";
}

void Response::setBody(const std::string &content) {
	/*
		setBody(content):
		1. Store body content
		2. Update Content-Length header
		3. Set Content-Type if not set
	*/
	_body = content;
    updateContentLength();
	if (_headers.find("Content-Type") == _headers.end()) {
        _headers["Content-Type"] = "text/html";
    }
}

void Response::appendBody(const std::string &content) {
	/*
		appendBody(content):
		1. Append content to existing body
		2. Update Content-Length header
		3. IF body was empty:
		   Set Content-Type if not set
		Note: Maintains response validity
	*/
	if (_body == "") {
		_body = content;
		if (_headers.find("Content-Type") == _headers.end()) {
			_headers["Content-Type"] = "text/html";
		}
	}
	else {
		_body += content;
	}
    updateContentLength();
}

const std::string &Response::getBody() const { return _body; }

std::string Response::toString() const {
	/*
		toString():
		1. Build status line
		2. Append formatted headers
		3. Add empty line
		4. Append body
		5. Return complete response string
	*/
	std::ostringstream response;
    response << "HTTP/1.1 " << _statusCode << " " << getStatusText() << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        response << formatHeader(it->first, it->second);
    }
    response << "\r\n" << _body;
    return response.str();
}

std::vector<unsigned char> Response::toBinary() const {
	/*
		toBinary():
		1. Convert string response to binary
		2. Handle special characters
		3. Return binary vector
	*/
	std::string response = toString();
    return std::vector<unsigned char>(response.begin(), response.end());
}

void Response::setCookie(const std::string &name, const std::string &value,
						 const std::map<std::string, std::string> &options) {
	/*
		setCookie(name, value, options):
		1. Validate cookie name/value
		2. Build cookie string with options:
		   - Expires
		   - Max-Age
		   - Domain
		   - Path
		   - Secure
		   - HttpOnly
		3. Add Set-Cookie header
	*/
    if (name.empty() || value.empty()) {
        throw std::invalid_argument("Cookie name or value cannot be empty");
    }
    std::ostringstream cookie;
    cookie << name << "=" << value;
    for (const auto &option : options) {
        if (option.first == "Expires" || option.first == "Max-Age" ||
            option.first == "Domain" || option.first == "Path" ||
            option.first == "Secure" || option.first == "HttpOnly") {
            cookie << "; " << option.first << "=" << option.second;
        }
    }
    addHeader("Set-Cookie", cookie.str());
}

void Response::redirect(const std::string &location, int code) {
	/*
		redirect(location, code):
		1. Set status code (301, 302, 307, 308)
		2. Add Location header
		3. Set minimal body
	*/
	setStatusCode(code);
    addHeader("Location", location);
    setBody("");
}

// Helper function for finding extenstion of a file
static bool endsWith(const std::string &str, const std::string &suffix) {
    if (suffix.size() > str.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

void Response::sendFile(const std::string &filePath) {
	/*
		sendFile(filePath):
		1. Determine content type
		2. Read file content
		3. Set appropriate headers
		4. Set body with file content
		
		5. Added handling if file is missing
	*/
	std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        setStatusCode(404);
        setBody("<h1>404 Not Found</h1>");
        addHeader("Content-Type", "text/html");
        addHeader("Content-Length", "23");
        return;
    }
    if (endsWith(filePath, ".html")) {
        addHeader("Content-Type", "text/html");
    } else if (endsWith(filePath, ".txt")) {
        addHeader("Content-Type", "text/plain");
    } else if (endsWith(filePath, ".json")) {
        addHeader("Content-Type", "application/json");
    } else if (endsWith(filePath, ".xml")) {
        addHeader("Content-Type", "application/xml");
    } else {
        addHeader("Content-Type", "application/octet-stream");
    }
    std::ostringstream content;
    content << file.rdbuf();
    addHeader("Content-Length", std::to_string(content.str().size()));
    setBody(content.str());
}

std::string Response::getStatusText() const {
	/*
		getStatusText():
		1. Look up status code
		2. Return corresponding text
		3. Handle unknown codes
	*/
	switch (_statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
		case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}

void Response::setDefaultHeaders() {
	/*
		setDefaultHeaders():
		1. Set Date header
		2. Set Server header
		3. Set Connection header
		4. Set Content-Type default
	*/
	_headers["Date"] = getCurrentDate();
    _headers["Server"] = "CustomServer/1.0";
    _headers["Connection"] = "keep-alive";
}

void Response::updateContentLength() {
	/*
		updateContentLength():
		1. Calculate body length
		2. Update Content-Length header
		3. Handle special cases:
		   - Chunked encoding
		   - Empty body
	*/
	if (_body.empty()) {
        _headers.erase("Content-Length");
        _headers["Transfer-Encoding"] = "chunked";
    } else {
        _headers["Content-Length"] = std::to_string(_body.size());
    }
}

bool Response::isValidStatusCode(int code) const {
	/*
		isValidStatusCode(code):
		1. Check if code is in valid range
		2. Verify code exists in HTTP spec
		3. Return validation result
	*/
	if (code < 100 || code >= 600) {
        return false;
    }
    const int validCodes[] = {
        200, 201, 204, 301, 302, 304, 400, 403, 404, 500, 502, 503
    };
    for (int i = 0; i < sizeof(validCodes) / sizeof(validCodes[0]); ++i) {
        if (code == validCodes[i]) {
            return true;
        }
    }
    return false;
}

std::string Response::formatHeader(const std::string &name, const std::string &value) const {
	/*
		formatHeader(name, value):
		1. Normalize header name
		2. Return "Name: value\r\n"
		Note: Used by toString() for consistent formatting
	*/
	std::string normalizedName = normalizeHeaderName(name);
    return normalizedName + ": " + value + "\r\n";
}
