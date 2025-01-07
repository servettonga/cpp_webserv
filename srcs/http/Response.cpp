/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:38:27 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

Response::Response(int statusCode, const std::string &serverName) :
		_statusCode(statusCode),
		_isRawOutput(false),
		_fileDescriptor(-1),
		_bytesWritten(0),
		_isStreaming(false),
		_isHeadersSent(false),
		_cookies() {
	_headers["Server"] = serverName;
	if (statusCode == 100) {
		_rawOutput = "HTTP/1.1 100 Continue\r\n\r\n";
		_isRawOutput = true;
	}
	setCookie("test_cookie", "webserv");
	setCookie("test_message", "hello");
}

Response::~Response() {
}

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
	if (name == "Set-Cookie") {
		std::string key = "Set-Cookie_" + Utils::numToString(_headers.size());
		_headers[key] = value;
	} else {
		_headers[name] = value;
	}
}

void Response::updateContentLength() {
	_headers["Content-Length"] = Utils::numToString(_body.length());
}

std::string Response::toString() const {
	if (_isRawOutput)
		return _rawOutput;
	std::string response = "HTTP/1.1 " + Utils::numToString(_statusCode) + " " + getStatusText() + "\r\n";

	response += "Content-Length: " + Utils::numToString(_body.length()) + "\r\n";

	// Add headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		if (it->first != "Content-Length" && it->first != "Set-Cookie")
			response += it->first + ": " + it->second + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		if (it->first == "Set-Cookie")
			response += "Set-Cookie: " + it->second + "\r\n";
	response += "\r\n";
	response += _body;
	return response;
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

Response Response::makeErrorResponse(int statusCode, const ServerConfig *config) {
	Response response(statusCode);
	response.addHeader("Content-Type", "text/html");

	if (config) {
		std::map<int, std::string>::const_iterator it = config->error_pages.find(statusCode);
		if (it != config->error_pages.end()) {
			std::string errorPath = config->root + it->second;
			struct stat st;
			if (stat(errorPath.c_str(), &st) == 0) {
				int fd = open(errorPath.c_str(), O_RDONLY);
				if (fd >= 0) {
					Response response(statusCode);
					response.addHeader("Content-Type", "text/html");
					response.addHeader("Content-Length", Utils::numToString(st.st_size));
					response.setFileDescriptor(fd);
					return response;
				}
			}
		}
	}

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

	std::string body = "<html><head><title>" + Utils::numToString(statusCode) + " " + response.getStatusText() +
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

void Response::setFileDescriptor(int fd) {
	closeFileDescriptor(); // Close existing fd if any
	_fileDescriptor = fd;
	_isStreaming = true;
	_bytesWritten = 0;
}

bool Response::writeNextChunk(int clientFd) {
	if (!_isStreaming || _fileDescriptor < 0)
		return false;

	try {
		// Send headers first if not sent
		if (!_isHeadersSent) {
			std::string headers = getHeadersString();
			ssize_t		headersSent = send(clientFd, headers.c_str(), headers.length(), MSG_NOSIGNAL);
			if (headersSent < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK)
					return true;
				return false;
			}
			_isHeadersSent = true;
		}

		// Send file content
		char	buffer[RESPONSE_SIZE];
		ssize_t bytesRead = read(_fileDescriptor, buffer, sizeof(buffer));

		if (bytesRead > 0) {
			ssize_t bytesWritten = send(clientFd, buffer, bytesRead, MSG_NOSIGNAL);
			if (bytesWritten < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK)
					return true;
				return false;
			}
			_bytesWritten += bytesWritten;
			return true;
		} else if (bytesRead == 0) { // EOF reached
			closeFileDescriptor();
			_isStreaming = false; // Mark streaming as complete
			return false;
		}
	} catch (const std::exception &e) {
		closeFileDescriptor();
		_isStreaming = false;
		return false;
	}
	return false;
}

void Response::closeFileDescriptor() {
	if (_fileDescriptor >= 0) {
		close(_fileDescriptor);
		_fileDescriptor = -1;
	}
}

void Response::setCookie(const std::string &name, const std::string &value, const std::string &expires,
						 const std::string &path) {
	std::string cookie = name + "=" + value;
	if (!expires.empty())
		cookie += "; Expires=" + expires;
	if (!path.empty())
		cookie += "; Path=" + path;
	_cookies[name] = cookie;
}

std::string Response::getHeadersString() const {
	std::string headers = "HTTP/1.1 " + Utils::numToString(_statusCode) + " " + getStatusText() + "\r\n";

	// Add regular headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		headers += it->first + ": " + it->second + "\r\n";

	// Add cookies
	for (std::map<std::string, std::string>::const_iterator it = _cookies.begin(); it != _cookies.end(); ++it)
		headers += "Set-Cookie: " + it->second + "\r\n";

	headers += "\r\n";
	return headers;
}

void Response::clearSession() {
	clearCookie("session_id");
}

void Response::clearCookie(const std::string &name) {
	if (name.empty())
		return;
	setCookie(name, "", "Thu, 01 Jan 1970 00:00:00 GMT", "/");
}

void Response::setSessionId(const std::string &sessionId) {
	if (!sessionId.empty())
		setCookie("session_id", sessionId, "", "/; Max-Age=3600");
}

Response Response::makeRedirect(int code, const std::string &location) {
	Response response(code);
	response.addHeader("Location", location);
	response.addHeader("Content-Type", "text/html");
	response.setBody("<html><body>Redirecting to " + location + "</body></html>");
	return response;
}
