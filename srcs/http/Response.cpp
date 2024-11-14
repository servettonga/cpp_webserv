/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/13 22:56:26 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include "../utils/Utils.hpp"
#include <algorithm>

using namespace Utils;

Response::Response() {
	/*
		Constructor:
		1. Set default status code (200)
		2. Initialize empty headers map
		3. Set default headers:
		   - Date
		   - Server
		   - Connection
	*/
	_statusCode = 200;
	setDefaultHeaders();
}

Response::Response(int statusCode) : _statusCode() {
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

Response::~Response() {

}

void Response::setStatusCode(int code) {
	/*
		setStatusCode(code):
		1. Validate status code
		2. Store status code
		3. Update status text
	*/
	if (code >= 100 && code < 600) {
		_statusCode = code;
	} else {
		_statusCode = 500;
		setBody("Internal Server Error");
	}
}

int Response::getStatusCode() const { return _statusCode; }

void Response::addHeader(const std::string &name, const std::string &value) {
	if (name.empty() || name.find_first_of("\r\n\0") != std::string::npos) // Skip invalid headers
		return ;
	_headers[name] = value;
}

void Response::removeHeader(const std::string &name) {
	std::string normalized = normalizeForComparison(name);
	// Remove header if it exists
	for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it) {
		if (it->first == normalized) {
			_headers.erase(it);
			if (normalized == "content-length" || normalized == "content-type") // Update internal state
				updateContentLength();
			return;
		}
	}
}

bool Response::hasHeader(const std::string &name) const {
	std::string normalized = normalizeForComparison(name); // For case-insensitive comparison
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		if (normalizeForComparison(it->first) == normalized)
			return true;
	return false;
}

std::string Response::getHeader(const std::string &name) const {
	std::string normalized = normalizeForComparison(name);
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		if (normalizeForComparison(it->first) == normalized)
			return it->second;
	return std::string("");
}

void Response::setBody(const std::string &content) {
	_body = content;
	updateContentLength();
	if (!hasHeader("Content-Type"))
		addHeader("Content-Type", "text/plain");
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
	(void)content;
}

const std::string &Response::getBody() const { return _body; }

std::string Response::formatHeader(const std::string &name, const std::string &value) {
	return (name + " " + value + "\r\n");
}

std::string Response::toString() const {
	/*
		toString():
		1. Build status line
		2. Append formatted headers
		3. Add empty line
		4. Append body
		5. Return complete response string
	*/
	std::string response;
	response.reserve(1024);
	// Status line (Only HTTP 1.1 compliant for now)
	response += "HTTP/1.1 " + StringUtils::numToString(_statusCode) + " " + getStatusText() + "\r\n";
	// Headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		response += it->first + ": " + it->second + "\r\n";
	// Empty line before body
	response += "\r\n";
	response += _body;
	return response;
}

std::vector<unsigned char> Response::toBinary() const {
	/*
		toBinary():
		1. Convert string response to binary
		2. Handle special characters
		3. Return binary vector
	*/
	return std::vector<unsigned char>();
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
	(void)name;
	(void)value;
	(void)options;
}

void Response::redirect(const std::string &location, int code) {
	/*
		redirect(location, code):
		1. Set status code (301, 302, 307, 308)
		2. Add Location header
		3. Set minimal body
	*/
	(void)location;
	(void)code;
}

void Response::sendFile(const std::string &filePath) {
	/*
		sendFile(filePath):
		1. Determine content type
		2. Read file content
		3. Set appropriate headers
		4. Set body with file content
	*/
	(void)filePath;
}

/*
 * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
 */
std::string Response::getStatusText() const {
	/*
		getStatusText():
		1. Look up status code
		2. Return corresponding text
		3. Handle unknown codes
	*/
	switch (_statusCode) {
		// 1xx informational response
		case 100: return "Continue";
		case 101: return "Switching Protocols";
		case 102: return "Processing";
		case 103: return "Early Hints";
		// 2xx success
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 207: return "Multi-Status";
		case 208: return "Already Reported";
		case 226: return "IM Used";
		// 3xx redirection
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 305: return "Use Proxy";
		case 306: return "Switch Proxy";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";
		// 4xx client errors
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 418: return "I'm a teapot";
		case 421: return "Misdirected Request";
		case 422: return "Unprocessable Content";
		case 423: return "Locked";
		case 424: return "Failed Dependency";
		case 425: return "Too Early";
		case 426: return "Upgrade Required";
		case 428: return "Precondition Required";
		case 429: return "Too Many Requests";
		case 431: return "Request Header Fields Too Large";
		case 451: return "Unavailable For Legal Reasons";
		case 494: return "Request Header Too Large"; // Nginx
		case 499: return "Client Closed Request"; // Nginx
		// 5xx server errors
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		case 506: return "Variant Also Negotiates";
		case 507: return "Insufficient Storage";
		case 508: return "Loop Detected";
		case 510: return "Not Extended";
		case 511: return "Network Authentication Required";
		default: return "Unknown Error";
	}
}

void Response::setDefaultHeaders() {
	_headers["Server"] = "webserv";
	_headers["Date"] = TimeUtils::getCurrentTime();
	_headers["Content-Type"] = "text/plain";
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
	// Remove any existing Content-Length
	for (std::map<std::string, std::string>::iterator it = _headers.begin();
		 it != _headers.end(); ++it) {
		if (normalizeForComparison(it->first) == "content-length") {
			_headers.erase(it);
			break;
		}
	}
	// Add new Content-Length if body not empty
	unsigned long length = _body.length();
	if (length > 0)
		addHeader("Content-Length", StringUtils::numToString(length));
}

std::string Response::normalizeForComparison(const std::string &name) {
	std::string normalized = name;
	for (std::string::iterator it = normalized.begin(); it != normalized.end(); ++it)
		*it = std::tolower(*it);
	return normalized;
}
