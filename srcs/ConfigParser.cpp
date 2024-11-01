/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:26:31 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 19:31:06 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <cstdlib>

/*
	Config Parser class implementation
 	- Reads the configuration file.
	- Parses configuration settings (to be implemented as needed).
 */

// Instantiate ConfigParser with the path to the configuration file
ConfigParser::ConfigParser(const std::string& configFilePath) : currentPosition(0) { parseConfigFile(configFilePath); }

ConfigParser::~ConfigParser() {}

const std::vector<ServerConfig>& ConfigParser::getServerConfigs() const { return serverConfigs; }

// *************** PARSING LOGIC ****************

// parseConfigFile reads the entire file content and initiates the parsing process
void ConfigParser::parseConfigFile(const std::string& configFilePath) {
	std::ifstream file(configFilePath.c_str());
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open configuration file: " + configFilePath);
	}

	// Read the entire file content into configContent
	std::stringstream buffer;
	buffer << file.rdbuf();
	configContent = buffer.str();
	file.close();

	// Start parsing from the beginning
	currentPosition = 0;

	// Parse the configuration content
	while (currentPosition < configContent.size()) {
		skipWhitespaceAndComments();
		std::string token = getNextToken();

		if (token == "server") {
			parseServerBlock();
		} else if (token.empty()) {
			// End of file
			break;
		} else {
			throwError("Unexpected token: " + token);
		}
	}
}

// TODO: add corresponding parsing logic in these methods to parse cgi_extension or redirect directives
// parseServerBlock and parseLocationBlock parse server and location blocks, respectively.
void ConfigParser::parseServerBlock() {
	ServerConfig serverConfig;

	expectToken("{");

	while (true) {
		skipWhitespaceAndComments();
		std::string token = getNextToken();

		if (token == "}") {
			// End of server block
			break;
		} else if (token == "listen") {
			// Parse listen directive
			skipWhitespaceAndComments();
			std::string portStr = getNextToken();
			serverConfig.port = std::atoi(portStr.c_str());
			expectToken(";");
		} else if (token == "server_name") {
			// Parse server_name directive
			skipWhitespaceAndComments();
			serverConfig.serverName = getNextToken();
			expectToken(";");
		} else if (token == "error_page") {
			// Parse error_page directive
			skipWhitespaceAndComments();
			std::string codeStr = getNextToken();
			int errorCode = std::atoi(codeStr.c_str());
			skipWhitespaceAndComments();
			std::string errorPage = getNextToken();
			serverConfig.errorPages[errorCode] = errorPage;
			expectToken(";");
		} else if (token == "client_max_body_size") {
			// Parse client_max_body_size directive
			skipWhitespaceAndComments();
			std::string sizeStr = getNextToken();
			serverConfig.clientMaxBodySize = std::atoi(sizeStr.c_str());
			expectToken(";");
		} else if (token == "location") {
			// Parse location block
			parseLocationBlock(serverConfig);
		} else {
			throwError("Unknown directive in server block: " + token);
		}
	}

	serverConfigs.push_back(serverConfig);
}

void ConfigParser::parseLocationBlock(ServerConfig& serverConfig) {
	LocationConfig locationConfig;

	// Get the path
	skipWhitespaceAndComments();
	locationConfig.path = getNextToken();

	expectToken("{");

	while (true) {
		skipWhitespaceAndComments();
		std::string token = getNextToken();

		if (token == "}") {
			// End of location block
			break;
		} else if (token == "root") {
			// Parse root directive
			skipWhitespaceAndComments();
			locationConfig.root = getNextToken();
			expectToken(";");
		} else if (token == "index") {
			// Parse index directive
			skipWhitespaceAndComments();
			locationConfig.index = getNextToken();
			expectToken(";");
		} else if (token == "autoindex") {
			// Parse autoindex directive
			skipWhitespaceAndComments();
			std::string value = getNextToken();
			locationConfig.autoindex = (value == "on");
			expectToken(";");
		} else if (token == "allowed_methods") {
			// Parse allowed_methods directive
			skipWhitespaceAndComments();
			while (true) {
				std::string method = getNextToken();
				if (method == ";") {
					break;
				}
				locationConfig.allowedMethods.push_back(method);
				skipWhitespaceAndComments();
			}
		} else if (token == "upload_path") {
			// Parse upload_path directive
			skipWhitespaceAndComments();
			locationConfig.uploadPath = getNextToken();
			expectToken(";");
		} else {
			throwError("Unknown directive in location block: " + token);
		}
	}

	serverConfig.locations.push_back(locationConfig);
}

// skipWhitespaceAndComments advances the parser position over whitespace and comments (lines starting with #)
void ConfigParser::skipWhitespaceAndComments() {
	while (currentPosition < configContent.size()) {
		char c = configContent[currentPosition];
		if (isspace(c)) {
			// Skip whitespace
			currentPosition++;
		} else if (c == '#') {
			// Skip comment until end of line
			while (currentPosition < configContent.size() && configContent[currentPosition] != '\n') {
				currentPosition++;
			}
		} else {
			break;
		}
	}
}

// *************** TOKENIZATION ****************

// getNextToken retrieves the next meaningful token from the configuration content
std::string ConfigParser::getNextToken() {
	skipWhitespaceAndComments();

	if (currentPosition >= configContent.size()) {
		return "";
	}

	char c = configContent[currentPosition];

	if (c == '{' || c == '}' || c == ';') {
		// Single character tokens
		currentPosition++;
		return std::string(1, c);
	} else if (isalnum(c) || c == '/' || c == '.' || c == '_' || c == '-') {
		// Alphanumeric tokens
		size_t start = currentPosition;
		while (currentPosition < configContent.size() &&
			   (isalnum(configContent[currentPosition]) || configContent[currentPosition] == '/' ||
				configContent[currentPosition] == '.' || configContent[currentPosition] == '_' ||
				configContent[currentPosition] == '-')) {
			currentPosition++;
		}
		return configContent.substr(start, currentPosition - start);
	} else if (c == '"') {
		// Quoted string
		currentPosition++;
		size_t start = currentPosition;
		while (currentPosition < configContent.size() && configContent[currentPosition] != '"') {
			currentPosition++;
		}
		std::string token = configContent.substr(start, currentPosition - start);
		if (currentPosition < configContent.size() && configContent[currentPosition] == '"') {
			currentPosition++; // Skip closing quote
		}
		return token;
	} else {
		// Unknown token
		throwError("Invalid character in configuration file: " + std::string(1, c));
		return "";
	}
}

void ConfigParser::expectToken(const std::string& expected) {
	std::string token = getNextToken();
	if (token != expected) {
		throwError("Expected token '" + expected + "', but found '" + token + "'");
	}
}

// throwError generates a std::runtime_error with a descriptive error message
void ConfigParser::throwError(const std::string &message) {
	throw std::runtime_error("ConfigParser Error: " + message);
}

/*
	TODO: Managing Configuration for Routes
	Enhance the ConfigParser to support route configurations.

	Features to Implement:
	- Define accepted HTTP methods per route.
	- Set up HTTP redirections.
	- Specify document roots for routes.
	- Enable or disable directory listing.
	- Define default files for directories.
	- Configure CGI execution for specific file extensions.
	- Specify upload locations for file uploads.
 */

/*
	TODO: Enforcing Client Body Size Limits
	Implement logic to enforce limits on client request body sizes.

	Actions:
	- Define a maximum body size in the configuration.
	- While reading the request body, track the size.
	- If the body exceeds the limit, send a 413 Payload Too Large response.
 */
