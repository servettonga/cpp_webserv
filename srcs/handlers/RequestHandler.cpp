/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/06 18:33:17 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"

RequestHandler::RequestHandler(const ServerConfig &config) : _config(config) {
	/*
		Constructor:
		1. Store server configuration
		2. Initialize CGI handler
	*/
	(void)_config;
}

RequestHandler::~RequestHandler() {

}

Response RequestHandler::handleRequest(const HTTPRequest &request) {
	/*
		handleRequest(request):
		1. Validate request basics (method, URI, version)
		2. Get matching location configuration
		3. Check if method is allowed
		4. IF CGI request:
		   - Route to CGI handler
		5. ELSE:
		   - Route to appropriate method handler
		6. IF error occurs:
		   - Generate error response
	*/
	(void)request;
	return Response();
}

Response RequestHandler::handleGET(const HTTPRequest &request) {
	/*
		handleGET(request):
		1. Validate request path
		2. IF directory:
		   - Check for index file
		   - IF no index and autoindex enabled:
			 Return directory listing
		   - ELSE:
			 Return 403 Forbidden
		3. IF file:
		   - Check permissions
		   - Get content type
		   - Read file
		   - Return response
		4. ELSE:
		   Return 404 Not Found
	*/
	(void)request;
	return Response();
}

Response RequestHandler::handlePOST(const HTTPRequest &request) {
	/*
		handlePOST(request):
		1. Validate request path
		2. Check content length limits
		3. IF CGI request:
		   - Route to CGI handler
		4. ELSE IF file upload:
		   - Validate file type
		   - Store file
		   - Return success response
		5. ELSE:
		   Return 400 Bad Request
	*/
	(void)request;
	return Response();
}

Response RequestHandler::handleDELETE(const HTTPRequest &request) {
	/*
		handleDELETE(request):
		1. Validate request path
		2. Check permissions
		3. IF file exists:
		   - Delete file
		   - Return success response
		4. ELSE:
		   Return 404 Not Found
	*/
	(void)request;
	return Response();
}

bool RequestHandler::isMethodAllowed(const std::string &method, const LocationConfig &loc) {
	/*
		isMethodAllowed(method, location):
		1. Get allowed methods from location config
		2. Check if method is in allowed list
		3. Return result
	*/
	(void)method;
	(void)loc;
	return false;
}

bool RequestHandler::isCGIRequest(const std::string &uri) {
	/*
		isCGIRequest(uri):
		1. Get file extension
		2. Check against CGI extensions
		3. Return result
	*/
	(void)uri;
	return false;
}

Response RequestHandler::serveStaticFile(const std::string &path) {
	/*
		serveStaticFile(path):
		1. Open file
		2. Read content
		3. Get content type
		4. Create response with file content
		5. Return response
	*/
	(void)path;
	return Response();
}

Response RequestHandler::handleDirectory(const std::string &path) {
	/*
		handleDirectory(path):
		1. Check for index file
		2. IF index exists:
		   - Serve index file
		3. ELSE IF autoindex enabled:
		   - Generate directory listing
		4. ELSE:
		   - Return 403 Forbidden
	*/
	(void)path;
	return Response();
}

Response RequestHandler::generateErrorResponse(int statusCode) {
	/*
		generateErrorResponse(statusCode):
		1. Check for custom error page
		2. IF custom page exists:
		   - Serve custom error page
		3. ELSE:
		   - Generate default error response
	*/
	(void)statusCode;
	return Response();
}

std::string RequestHandler::getContentType(const std::string &path) {
	/*
		getContentType(path):
		1. Get file extension
		2. Look up MIME type
		3. Return content type
	*/
	(void)path;
	return std::string();
}

bool RequestHandler::validatePath(const std::string &path) {
	/*
		validatePath(path):
		1. Check for path traversal
		2. Verify path is within root
		3. Check file exists
		4. Check permissions
		5. Return validation result
	*/
	(void)path;
	return false;
}

const LocationConfig *RequestHandler::getLocation(const std::string &uri) {
	/*
		getLocation(uri):
		1. Find most specific matching location
		2. Return location config
		3. Return NULL if no match
	*/
	(void)uri;
	return NULL;
}
