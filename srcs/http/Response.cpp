/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/24 19:33:36 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include "../utils/Utils.hpp"
#include <algorithm>
#include <cstdio>
#include <sstream>

using namespace Utils;

Response::Response(int statusCode, const std::string &serverName) : _statusCode(statusCode) {
	_headers["Server"] = serverName;
	_headers["Content-Type"] = "text/plain";
}

void Response::setStatusCode(int code) {
	if (code >= 100 && code < 600)
		_statusCode = code;
}

void Response::setBody(const std::string& body) {
	_body = body;
	updateContentLength();
}

void Response::addHeader(const std::string& name, const std::string& value) {
	if (name.empty() || name.find_first_of("\r\n\0") != std::string::npos) // Skip invalid headers
		return ;
	_headers[name] = value;
}

void Response::updateContentLength() {
	std::map<std::string, std::string>::iterator it;
	for (it = _headers.begin(); it != _headers.end(); ++it) {
		std::string key = it->first;
		for (std::string::iterator c = key.begin(); c != key.end(); ++c)
			*c = std::tolower(*c);
		if (key == "content-length") {
			char buf[32];
			sprintf(buf, "%lu", _body.length());
			it->second = buf;
			return;
		}
	}
	_headers["Content-Length"] = StringUtils::numToString(_body.length());
}

std::string Response::toString() const {
	std::stringstream response;

	// Status line
	response << "HTTP/1.1 " << _statusCode << " " << getStatusText() << "\r\n";

	// Add Content-Length if not present
	bool hasContentLength = false;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it) {
		response << it->first << ": " << it->second << "\r\n";
		if (it->first == "Content-Length")
			hasContentLength = true;
	}

	// Add Content-Length if missing
	if (!hasContentLength)
		response << "Content-Length: " << _body.length() << "\r\n";

	// Empty line
	response << "\r\n";
	// Body
	response << _body;

	return response.str();
}

/*
 * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
 */
std::string Response::getStatusText() const {
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
		default: return "Unknown";
	}
}

Response::~Response() {}
