/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/03 14:18:07 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include "../utils/Utils.hpp"
#include <sstream>

Response::Response(int statusCode, const std::string &serverName) :
	_statusCode(statusCode), _isChunked(false) {
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
	_headers["Content-Length"] = Utils::numToString(_body.length());
}

std::string Response::toString() const {
	std::stringstream response;

	// Status line
	response << "HTTP/1.1 " << _statusCode << " " << getStatusText() << "\r\n";

	// Headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it) {
		// Skip debug header and Content-Length if chunked
		if (it->first != "Executing CGI handler" &&
			(!_isChunked || it->first != "Content-Length")) {
			response << it->first << ": " << it->second << "\r\n";
		}
	}

	// Add Transfer-Encoding header if chunked
	if (_isChunked)
		response << "Transfer-Encoding: chunked\r\n";

	response << "\r\n";

	// Body
	if (_isChunked) {
		std::string chunk;
		size_t pos = 0;
		const size_t chunkSize = 1024; // 1KB chunks

		while (pos < _body.length()) {
			size_t len = std::min(chunkSize, _body.length() - pos);
			chunk = _body.substr(pos, len);

			// Write chunk size in hex
			std::stringstream hexLen;
			hexLen << std::hex << chunk.length();
			response << hexLen.str() << "\r\n";

			// Write chunk data
			response << chunk << "\r\n";
			pos += len;
		}
		// Write final chunk
		response << "0\r\n\r\n";
	} else {
		response << _body;
	}

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

Response Response::makeErrorResponse(int statusCode) {
	Response response(statusCode);
	response.addHeader("Content-Type", "text/html");

	// Map of custom error messages
	std::map<int, std::string> errorMessages;
	errorMessages[400] = "The request could not be understood by the server.";
	errorMessages[401] = "Authentication is required and has failed or not been provided.";
	errorMessages[403] = "You don't have permission to access this resource.";
	errorMessages[404] = "The requested resource could not be found on this server.";
	errorMessages[405] = "The requested method is not allowed for this resource.";
	errorMessages[413] = "The request entity is larger than the server is willing to process.";
	errorMessages[415] = "The server does not support the media type of the requested data.";
	errorMessages[500] = "The server encountered an unexpected condition.";
	errorMessages[501] = "The server does not support the functionality required.";
	errorMessages[502] = "The server received an invalid response from an upstream server.";
	errorMessages[503] = "The server is temporarily unable to handle the request.";

	// Determine error category for styling
	std::string colorClass = (statusCode >= 500) ? "#ffebee" : "#fff3e0";
	std::string buttonColor = (statusCode >= 500) ? "#f44336" : "#ff9800";

	std::string message = errorMessages[statusCode];
	if (message.empty()) {
		message = "An error occurred while processing your request.";
	}

        std::string body =
            "<html><head><title>" + Utils::numToString(statusCode) + " " +
            response.getStatusText() +
            "</title>"
            "<style>"
            "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; "
            "background-color: " +
            colorClass +
            "; }"
            ".container { text-align: center; padding: 30px; max-width: 800px; "
            "margin: 0 auto; }"
            "h1 { color: #333; margin: 20px 0; }"
            ".error-code { font-size: 72px; color: #666; margin: 20px 0; }"
            ".message { color: #666; margin: 30px 0; line-height: 1.5; }"
            ".home-link { display: inline-block; padding: 10px 20px; "
            "background-color: " +
            buttonColor +
            ";"
            "    color: white; text-decoration: none; border-radius: 4px; "
            "margin-top: 30px; }"
            ".home-link:hover { opacity: 0.9; }"
            "</style></head>"
            "<body>\n"
            "    <div class='container'>\n"
            "        <div class='error-code'>" +
            Utils::numToString(statusCode) +
            "</div>\n"
            "        <h1>" +
            response.getStatusText() +
            "</h1>\n"
            "        <p class='message'>" +
            message +
            "</p>\n"
            "    </div>\n"
            "</body></html>";

        response.setBody(body);
        return response;
}

void Response::setChunked(bool chunked) { _isChunked = chunked; }

bool Response::isChunked() const { return _isChunked; }
