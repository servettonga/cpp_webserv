/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 17:19:04 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include "../utils/Utils.hpp"
#include <sstream>

using namespace Utils;

Response::Response(int statusCode, const std::string &serverName) :
	_statusCode(statusCode) {
	_headers["Server"] = serverName;
	_headers["Content-Type"] = "text/plain";
}

Response::~Response() {}

void Response::setStatusCode(int code) {
	if (code >= 100 && code < 600)
		_statusCode = code;
}

void Response::setBody(const std::string &body) {
	_body = body;
	updateContentLength();
}

void Response::addHeader(const std::string &name, const std::string &value) {
	if (name.empty() || name.find_first_of("\r\n\0") != std::string::npos)
		return;
	_headers[name] = value;
}

void Response::updateContentLength() {
	_headers["Content-Length"] = StringUtils::numToString(_body.length());
}

std::string Response::toString() const {
	std::stringstream response;

	// Status line
	response << "HTTP/1.1 " << _statusCode << " " << getStatusText() << "\r\n";

	// Headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it) {
		response << it->first << ": " << it->second << "\r\n";
	}

	// Empty line and body
	response << "\r\n" << _body;

	return response.str();
}

std::string Response::getStatusText() const {
	switch (_statusCode) {
		// 2xx Success
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";

		// 3xx Redirection
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 304: return "Not Modified";

		// 4xx Client Errors
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 415: return "Unsupported Media Type";

		// 5xx Server Errors
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";

		default: return "Unknown";
	}
}
