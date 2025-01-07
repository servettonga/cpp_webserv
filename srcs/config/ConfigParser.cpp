/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:35:35 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "../utils/Utils.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

ConfigParser::ConfigParser(const std::string &configPath) : _configPath(configPath), _currentLine(0) {
}

ConfigParser::~ConfigParser() {
}

std::vector<ServerConfig> ConfigParser::parse() {
	std::vector<ServerConfig> configs;
	readConfigFile();
	_errors.clear();
	_currentLine = 0;

	while (hasMoreLines()) {
		skipWhitespace();
		if (!hasMoreLines())
			break;

		std::string line = getCurrentLine();

		if (line.find("server") != std::string::npos) {
			ServerConfig config;
			parseServer(config);
			configs.push_back(config);
		} else if (line == "cgi {") {
			if (!configs.empty())
				parseCGI(configs.back());
			else
				addError("CGI block must be inside or after a server block");
		} else if (!line.empty() && line[0] != '#') {
			addError("Unexpected directive outside server block");
			++_currentLine;
		} else {
			++_currentLine;
		}
	}

	return configs;
}

void ConfigParser::readConfigFile() {
	std::ifstream file(_configPath.c_str());
	if (!file.is_open()) {
		addError("Failed to open config file: " + _configPath);
		return;
	}

	_configLines.clear();
	std::string line;

	while (std::getline(file, line)) {
		// Remove comments
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);
		line = trimString(line);
		if (!line.empty())
			_configLines.push_back(line);
	}

	file.close();
}

void ConfigParser::parseServer(ServerConfig &server) {
	if (!hasMoreLines()) {
		addError("Unexpected end of file in server block");
		return;
	}

	std::string line = getCurrentLine();

	// Skip the server line and opening brace
	if (line.find("server") != std::string::npos) {
		++_currentLine;
		skipWhitespace();
		if (hasMoreLines() && isBlockStart(getCurrentLine()))
			++_currentLine;
	}

	while (hasMoreLines()) {
		skipWhitespace();
		if (!hasMoreLines())
			break;

		line = getCurrentLine();
		if (line == "}") {
			++_currentLine;
			break;
		}
		if (line.find("location") == 0) {
			LocationConfig location;
			parseLocation(location);
			server.locations.push_back(location);
			continue;
		}
		if (line.find("cgi") == 0 && line.find("{") != std::string::npos) {
			parseCGI(server);
			continue;
		}
		parseDirective(line, server);
		++_currentLine;
	}
}

bool ConfigParser::isBlockStart(const std::string &line) const {
	std::string trimmed = trimString(line);
	return trimmed == "{" || trimmed == "server {";
}

bool ConfigParser::isBlockEnd(const std::string &line) const {
	std::string trimmed = trimString(line);
	return trimmed == "}";
}

std::string ConfigParser::trimString(const std::string &str) const {
	if (str.empty())
		return str;

	size_t start = 0;
	size_t end = str.length();

	while (start < end && isspace(str[start])) ++start;
	while (end > start && isspace(str[end - 1])) --end;

	return str.substr(start, end - start);
}

void ConfigParser::parseLocation(LocationConfig &location) {
	std::string line = getCurrentLine();

	std::istringstream iss(line);
	std::string		   token;
	iss >> token; // Skip "location"
	iss >> token; // Get path
	location.path = token;

	while (hasMoreLines()) {
		if (getCurrentLine().find("{") != std::string::npos) {
			++_currentLine;
			break;
		}
		++_currentLine;
	}

	location.root = "www";
	location.methods.push_back("GET");
	location.autoindex = false;

	while (hasMoreLines()) {
		skipWhitespace();
		line = getCurrentLine();

		if (line == "}") {
			++_currentLine;
			break;
		}
		std::pair<std::string, std::string> directive = splitDirective(line);
		std::string							value = directive.second;
		if (!value.empty() && value[value.length() - 1] == ';')
			value = value.substr(0, value.length() - 1);
		if (directive.first == "root")
			location.root = value;
		else if (directive.first == "index")
			location.index = value;
		else if (directive.first == "autoindex")
			location.autoindex = (value == "on");
		else if (directive.first == "client_max_body_size")
			location.client_max_body_size = parseSize(value);
		else if (directive.first == "allowed_methods") {
			location.methods.clear();
			parseAllowedMethods(value, location);
		} else if (directive.first == "cgi_pass") {
			location.cgi_path = value;
		} else if (directive.first == "return") {
			std::istringstream iss(value);
			std::string		   code, target;
			iss >> code >> target;
			if (code == "301" || code == "302") {
				if (!target.empty() && target[target.length() - 1] == ';')
					target = target.substr(0, target.length() - 1);
				location.redirect = target;
			}
		}
		++_currentLine;
	}
}

void ConfigParser::parseCGI(ServerConfig &server) {
	++_currentLine;

	while (hasMoreLines()) {
		skipWhitespace();
		std::string line = getCurrentLine();

		if (isBlockEnd(line)) {
			++_currentLine;
			break;
		}

		std::pair<std::string, std::string> directive = splitDirective(line);
		std::string							ext = directive.first;
		std::string							handler = directive.second;

		if (ext[0] != '.')
			ext = "." + ext;
		if (!handler.empty() && handler[handler.length() - 1] == ';')
			handler = handler.substr(0, handler.length() - 1);
		server.cgi_handlers[ext] = handler;
		++_currentLine;
	}
}

void ConfigParser::parseDirective(const std::string &line, ServerConfig &server) {
	std::string directiveLine = line;
	if (!directiveLine.empty() && directiveLine[directiveLine.length() - 1] == ';')
		directiveLine = directiveLine.substr(0, directiveLine.length() - 1);

	std::pair<std::string, std::string> directive = splitDirective(directiveLine);

	if (directive.first == "host")
		server.host = directive.second;
	else if (directive.first == "port")
		server.port = atoi(directive.second.c_str());
	else if (directive.first == "server_name")
		parseServerNames(directive.second, server);
	else if (directive.first == "root")
		server.root = directive.second;
	else if (directive.first == "index")
		server.index = directive.second;
	else if (directive.first == "client_timeout")
		server.client_timeout = atoi(directive.second.c_str());
	else if (directive.first == "client_max_body_size")
		server.client_max_body_size = parseSize(directive.second);
	else if (directive.first == "error_page")
		parseErrorPage(directive.second, server);
}

std::pair<std::string, std::string> ConfigParser::splitDirective(const std::string &line) const {
	std::string key, value;
	size_t		pos = line.find_first_of(" \t");

	if (pos != std::string::npos) {
		key = trimString(line.substr(0, pos));
		value = trimString(line.substr(pos + 1));
	} else {
		key = trimString(line);
	}

	return std::make_pair(key, value);
}

void ConfigParser::skipWhitespace() {
	while (hasMoreLines() && trimString(getCurrentLine()).empty()) ++_currentLine;
}

bool ConfigParser::hasMoreLines() const {
	return _currentLine < _configLines.size();
}

std::string ConfigParser::getCurrentLine() const {
	return hasMoreLines() ? _configLines[_currentLine] : "";
}

void ConfigParser::addError(const std::string &error) {
	std::ostringstream oss;
	oss << "Line " << (_currentLine + 1) << ": " << error;
	_errors.push_back(oss.str());
}

bool ConfigParser::validatePaths(const ServerConfig &config) const {
	struct stat				 st;
	std::vector<std::string> pathsToCheck;

	// Add server to the root path
	if (!config.root.empty())
		pathsToCheck.push_back(config.root);
	// Add location paths
	for (std::vector<LocationConfig>::const_iterator it = config.locations.begin(); it != config.locations.end();
		 ++it) {
		if (!it->root.empty()) {
			std::string locPath = it->root;
			if (!locPath.empty() && locPath[locPath.length() - 1] == ';')
				locPath = locPath.substr(0, locPath.length() - 1);
			if (locPath.substr(0, 2) == "./")
				locPath = locPath.substr(2);
			pathsToCheck.push_back(locPath);
		}
	}
	// Validate each path
	for (std::vector<std::string>::const_iterator it = pathsToCheck.begin(); it != pathsToCheck.end(); ++it) {
		if (stat(it->c_str(), &st) != 0) {
			std::string cmd = "mkdir -p \"" + *it + "\"";
			int			result = system(cmd.c_str());
			if (result != 0) {
				std::cerr << "Failed to create directory: " << *it << std::endl;
				return false;
			}
			std::cout << "Created directory: " << *it << std::endl;
		}
	}
	return true;
}

bool ConfigParser::validatePorts(const ServerConfig &config) const {
	return config.port > 0 && config.port < 65536;
}

bool ConfigParser::validateCGI(const ServerConfig &config) const {
	for (std::map<std::string, std::string>::const_iterator it = config.cgi_handlers.begin();
		 it != config.cgi_handlers.end(); ++it) {
		std::string handler = it->second;
		if (!handler.empty() && handler[handler.length() - 1] == ';')
			handler = handler.substr(0, handler.length() - 1);

		// If using env
		if (handler.find("/usr/bin/env") == 0) {
			std::istringstream iss(handler);
			std::string		   env, cmd;
			iss >> env >> cmd;

			if (!cmd.empty() && cmd[cmd.length() - 1] == ';')
				cmd = cmd.substr(0, cmd.length() - 1);

			// Check common paths
			std::vector<std::string> paths;
			paths.push_back("/usr/bin/" + cmd);
			paths.push_back("/usr/local/bin/" + cmd);
			paths.push_back("/bin/" + cmd);

			bool found = false;
			for (size_t i = 0; i < paths.size(); ++i) {
				struct stat st;
				if (stat(paths[i].c_str(), &st) == 0 && (st.st_mode & S_IXUSR)) {
					found = true;
					break;
				}
			}

			if (!found)
				return false;
		} else {
			struct stat st;
			if (stat(handler.c_str(), &st) != 0 || !(st.st_mode & S_IXUSR))
				return false;
		}
	}
	return true;
}

bool ConfigParser::validateLocations(const ServerConfig &config) const {
	std::vector<std::string> paths;

	for (std::vector<LocationConfig>::const_iterator it = config.locations.begin(); it != config.locations.end();
		 ++it) {
		if (std::find(paths.begin(), paths.end(), it->path) != paths.end())
			return false;
		paths.push_back(it->path);

		// Don't fail if methods are empty - they might be inherited from server config
		// Just warn about it
		if (it->methods.empty())
			std::cout << "Warning: No methods specified for location " << it->path << std::endl;
	}
	return true;
}

std::vector<std::string> ConfigParser::getErrors() const {
	return _errors;
}

void ConfigParser::reload() {
	_currentLine = 0;
	_errors.clear();
	readConfigFile();
}

void ConfigParser::parseServerNames(const std::string &value, ServerConfig &server) {
	std::istringstream iss(value);
	std::string		   name;
	while (iss >> name) server.server_names.push_back(name);
}

void ConfigParser::parseErrorPage(const std::string &value, ServerConfig &server) {
	std::istringstream iss(value);
	std::string		   code_str, path;

	if (iss >> code_str >> path) {
		int code = atoi(code_str.c_str());
		if (code >= 400 && code < 600) // Valid HTTP error codes
			server.error_pages[code] = path;
		else
			addError("Invalid error code in error_page directive: " + code_str);
	} else {
		addError("Invalid error_page directive syntax");
	}
}

void ConfigParser::parseAllowedMethods(const std::string &value, LocationConfig &location) {
	std::string methodStr = value;
	if (!methodStr.empty() && methodStr[methodStr.length() - 1] == ';')
		methodStr = methodStr.substr(0, methodStr.length() - 1);

	std::istringstream iss(methodStr);
	std::string		   method;

	while (iss >> method) {
		if (method[method.length() - 1] == ';')
			method = method.substr(0, method.length() - 1);
		if (method == "GET" || method == "POST" || method == "DELETE" || method == "PUT" || method == "HEAD") {
			location.methods.push_back(method);
		}
	}

	if (location.methods.empty())
		location.methods.push_back("GET");
}

unsigned long ConfigParser::parseSize(const std::string &value) {
	std::string number;
	std::string unit;
	size_t		i = 0;

	// Parse number part
	while (i < value.length() && (isdigit(value[i]) || value[i] == '.')) number += value[i++];

	// Parse unit part
	while (i < value.length() && isalpha(value[i])) unit += tolower(value[i++]);

	double size = atof(number.c_str());

	if (unit == "k" || unit == "kb") {
		size *= 1024;
	} else if (unit == "m" || unit == "mb") {
		size *= 1024 * 1024;
	} else if (unit == "g" || unit == "gb") {
		size *= 1024 * 1024 * 1024;
	} else if (!unit.empty()) {
		addError("Invalid size unit in directive: " + unit);
		return 0;
	}

	return (unsigned long)size;
}

bool ConfigParser::validate() {
	bool isValid = true;
	_errors.clear();

	try {
		std::vector<ServerConfig> configs = parse();

		// Check for duplicate ports
		std::map<int, std::string> usedPorts;
		for (std::vector<ServerConfig>::const_iterator it = configs.begin(); it != configs.end(); ++it) {
			std::string serverName = it->server_names.empty() ? it->host : it->server_names[0];

			if (usedPorts.find(it->port) != usedPorts.end()) {
				addError("Duplicate port " + Utils::numToString(it->port) + " used by servers " + usedPorts[it->port] +
						 " and " + serverName);
				isValid = false;
			}
			usedPorts[it->port] = serverName;
		}

		// Validate each config
		for (std::vector<ServerConfig>::const_iterator it = configs.begin(); it != configs.end(); ++it) {
			if (!validatePaths(*it)) {
				addError("Invalid paths in server " + (it->server_names.empty() ? it->host : it->server_names[0]));
				isValid = false;
			}
			if (!validatePorts(*it)) {
				addError("Invalid port configuration");
				isValid = false;
			}
			if (!validateCGI(*it)) {
				addError("Invalid CGI configuration");
				isValid = false;
			}
			if (!validateLocations(*it)) {
				addError("Invalid location configuration");
				isValid = false;
			}
		}
	} catch (const std::exception &e) {
		addError(std::string("Validation error: ") + e.what());
		std::cout << "Validation error: " << e.what() << std::endl;
		isValid = false;
	}

	return isValid;
}
