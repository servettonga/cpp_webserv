/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/06 18:46:25 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

ConfigParser::ConfigParser(const std::string &configPath) : _currentLine() {
	/*
		Constructor:
		1. Store config file path
		2. Initialize parser state
		3. Clear error list
	*/
	_configPath = configPath;
	_configLines = std::vector<std::string>();
	_currentLine = 0;
	_errors = std::vector<std::string>();
	_state = MAIN;
}

ConfigParser::~ConfigParser() {

}

std::vector<ServerConfig> ConfigParser::parse() {
	/*
		parse():
		1. Read config file
		2. Clear previous configs
		3. WHILE hasMoreLines():
		   - Skip whitespace and comments
		   - IF server block found:
			 Parse server block
		   - ELSE:
			 Handle invalid directive
		4. Validate configurations
		5. Return parsed configs
	*/
	return std::vector<ServerConfig>();
}

void ConfigParser::reload() {
	/*
		reload():
		1. Clear all internal state:
		   - Clear errors vector
		   - Clear current line
		   - Reset line counter
		2. Try:
		   - Read config file
		   - Parse configurations
		   - Validate configurations
		3. Catch any errors:
		   - Add error message
		   - Return false
		4. Return true if successful
	*/
}

bool ConfigParser::validate() {
	/*
		validate():
		1. Clear previous errors
		2. FOR each server config:
		   - Validate paths
		   - Validate ports
		   - Validate CGI
		   - Validate locations
		3. Return true if no errors
	*/
	return false;
}

std::vector<std::string> ConfigParser::getErrors() {
	/*
		getErrors():
		1. Return vector of error messages
		Note: Each error should include:
		- Line number (if applicable)
		- Error description
		- Context (directive or block where error occurred)
	*/
	return std::vector<std::string>();
}

void ConfigParser::readConfigFile() {
	/*
		readConfigFile():
		1. Open config file
		2. Read lines into vector
		3. Handle file errors
		4. Remove comments
		5. Trim whitespace
	*/
}

void ConfigParser::parseServer(ServerConfig &server) {
	/*
		parseServer(server):
		1. Initialize new server config
		2. WHILE in server block:
		   IF location block:
			 Parse location
		   ELSE IF CGI block:
			 Parse CGI
		   ELSE:
			 Parse directive
		3. Validate server config

	 	Example:
	 	void ConfigParser::parseServer(ServerConfig &server) {
			skipWhitespace();
			if (!hasMoreLines()) {
				addError("Unexpected end of file in server block");
				return;
			}
			std::string line = getCurrentLine();
			if (!isBlockStart(line)) {
				addError("Expected '{' at start of server block");
				return;
			}
			while (hasMoreLines()) {
				skipWhitespace();
				line = getCurrentLine();
				if (isBlockEnd(line))
					return;
				// Parse server directives...
			}
			addError("Unterminated server block");
		}
	*/
	(void)server;
}

void ConfigParser::parseLocation(LocationConfig &location) {
	/*
		parseLocation(location):
		1. Get location path
		2. WHILE in location block:
		   - Parse location directives:
			 - root
			 - index
			 - methods
			 - cgi
			 - autoindex
			 - client_max_body_size
		3. Validate location config
	*/
	(void)location;
}

void ConfigParser::parseCGI(ServerConfig &server) {
	/*
		parseCGI(server):
		1. WHILE in CGI block:
		   - Parse CGI directives:
			 - extensions
			 - handlers
			 - timeout
		2. Validate CGI config
	*/
	(void)server;
}

void ConfigParser::parseDirective(const std::string &line, ServerConfig &server) {
	/*
		parseDirective(line, server):
		1. Split directive into key/value
		2. Handle different directives:
		   - listen
		   - server_name
		   - root
		   - index
		   - error_page
		   - client_max_body_size
		   - timeout
		3. Validate directive value

	 	Example:
	 	void ConfigParser::parseDirective(const std::string &line, ServerConfig &server) {
			std::pair<std::string, std::string> directive = splitDirective(line);
			if (directive.first.empty()) {
				addError("Empty directive");
				return;
			}
			if (directive.first == "listen")
				server.setPort(atoi(directive.second.c_str()));
			else if (directive.first == "server_name")
				server.setServerName(directive.second);
			// ... handle other directives
		}

	 	Example valid directives:
		# Simple directive
		listen 8080

		# Directive with multiple values
		index index.html index.htm

		# Directive with quoted value
		root "/var/www/html"

		# Directive with comment
		server_name example.com	# primary domain
	*/
	(void)line;
	(void)server;
}

bool ConfigParser::isBlockStart(const std::string &line) {
	/*
		isBlockStart(line):
		1. Skip leading whitespace
		2. IF line empty:
		   Return false
		3. Find last non-whitespace char
		4. Return true if char is '{'
		5. Return false otherwise
	*/
	(void)line;
	return false;
}

bool ConfigParser::isBlockEnd(const std::string &line) {
	/*
		isBlockEnd(line):
		1. Skip leading whitespace
		2. IF line empty:
		   Return false
		3. Find last non-whitespace char
		4. Return true if char is '}'
		5. Return false otherwise
	*/
	(void)line;
	return false;
}

std::pair<std::string, std::string> ConfigParser::splitDirective(const std::string &line) {
	/*
		splitDirective(line):
		1. Skip leading whitespace
		2. IF line is empty or comment:
		   Return an empty pair

		3. Find first whitespace after directive name:
		   position = find_first_of(" \t", start)
		   IF no whitespace found:
			 Return pair(line, "")

		4. Extract directive name:
		   name = line.substr(0, position)

		5. Skip whitespace after name:
		   WHILE position < length AND isspace(line[position]):
			 position++

		6. Extract value:
		   IF position < length:
			 value = line.substr(position)
			 Remove trailing whitespace from value
		   ELSE:
			 value = ""

		7. Return pair(name, value)

		Note: Handle special cases:
		- Quoted values (preserve spaces within quotes)
		- Multiple values (e.g., "index file1.html file2.html")
		- Comments after values
		- Escaped characters
	*/
	(void)line;
	return std::pair<std::string, std::string>();
}

void ConfigParser::skipWhitespace() {
	/*
		skipWhitespace():
		1. WHILE hasMoreLines() AND current line is whitespace:
		   - Move to next line
		2. IF at valid line:
		   - Trim leading/trailing whitespace
		Note: Whitespace includes:
		- Empty lines
		- Lines with only spaces/tabs
		- Comment lines (starting with #)
	*/
}

bool ConfigParser::hasMoreLines() {
	/*
		hasMoreLines():
		1. Check if current line index < total lines
		2. Return true if more lines available
		3. Return false if at end of file
		Note: This tracks position in the loaded config file
	*/
	return false;
}

std::string ConfigParser::getCurrentLine() {
	/*
		getCurrentLine():
		1. IF no more lines:
		   Return empty string
		2. Return current line from loaded lines
		Note: Returns trimmed line without comments
	*/
	return std::string();
}

void ConfigParser::addError(const std::string &error) {
	/*
		addError(error):
		1. Format error message with:
		   - Line number (if available)
		   - Error message
		   - Context information
		2. Add to errors vector
		Note: Used throughout parsing to collect all errors
	*/
	(void)error;
}

bool ConfigParser::validatePaths(const ServerConfig &config) {
	/*
		validatePaths(config):
		1. Check root directory exists
		2. Check error pages exist
		3. Check CGI handlers exist
		4. Check location paths
		5. Return validation result
	*/
	(void)config;
	return false;
}

bool ConfigParser::validatePorts(const ServerConfig &config) {
	/*
		validatePorts(config):
		1. Check port number range
		2. Check port availability
		3. Check for duplicates
		4. Return validation result
	*/
	(void)config;
	return false;
}

bool ConfigParser::validateCGI(const ServerConfig &config) {
	/*
		validateCGI(config):
		1. Check handler paths
		2. Validate extensions
		3. Check permissions
		4. Return validation result
	*/
	(void)config;
	return false;
}

bool ConfigParser::validateLocations(const ServerConfig &config) {
	/*
		validateLocations(config):
		1. Check for path conflicts
		2. Validate methods
		3. Check directory permissions
		4. Return validation result
	*/
	(void)config;
	return false;
}
