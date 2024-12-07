/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 13:04:01 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/07 18:56:23 by jdepka           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

ConfigParser::ConfigParser(const std::string &configPath) : _configPath(configPath), _currentLine(0), _state(MAIN) {
	/*
		Constructor:
		1. Store config file path
		2. Initialize parser state
		3. Clear error list
	*/
	_configLines = std::vector<std::string>();
	_errors.clear();
}

ConfigParser::~ConfigParser() {}

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
	
	std::vector<ServerConfig> serverConfigs;

	readConfigFile();

	_errors.clear();

	while (hasMoreLines()) {
		skipWhiteSpace();
		if (!hasMoreLines()) {
			break;
		}

		std::string line = getCurrentLine();

		if (isBlockStart(line)) {
			ServerConfig serverConfig;
			parseServer(serverConfig);
			serverConfigs.push_back(serverConfig);
		} else {
			addError("Unexpected directive outside of server block: " + line);
		}
	}

	for (size_t i = 0; i < serverConfigs.size(); ++i) {
        if (!validatePaths(serverConfigs[i])) {
            addError("Invalid paths in server block " + Utils::StringUtils::numToString(i + 1));
        }
        if (!validatePorts(serverConfigs[i])) {
            addError("Invalid ports in server block " + Utils::StringUtils::numToString(i + 1));
        }
        if (!validateCGI(serverConfigs[i])) {
            addError("Invalid CGI configuration in server block " + Utils::StringUtils::numToString(i + 1));
        }
        if (!validateLocations(serverConfigs[i])) {
            addError("Invalid location settings in server block " + Utils::StringUtils::numToString(i + 1));
        }
    }

	return serverConfigs;
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

    _errors.clear();
    _configLines.clear();
    _currentLine = 0;

    try {
        readConfigFile();
        std::vector<ServerConfig> configs = parse();
        for (std::vector<ServerConfig>::iterator it = configs.begin(); it != configs.end(); ++it) {
            if (!validatePaths(*it) || !validatePorts(*it) || !validateCGI(*it) || !validateLocations(*it)) {
                addError("Validation failed for one or more server configurations");
                return;
            }
        }
    } catch (const std::exception &e) {
        addError(std::string("Exception occurred: ") + e.what());
    }
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
    _errors.clear();

    std::vector<ServerConfig> serverConfigs = parse();
    bool isValid = true;
    for (std::vector<ServerConfig>::iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it) {
        if (!validatePaths(*it)) {
            addError("Path validation failed for server");
            isValid = false;
        }

        if (!validatePorts(*it)) {
            addError("Port validation failed for server");
            isValid = false;
        }

        if (!validateCGI(*it)) {
            addError("CGI validation failed for server");
            isValid = false;
        }

        if (!validateLocations(*it)) {
            addError("Location validation failed for server");
            isValid = false;
        }
    }

    return isValid;
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
	std::vector<std::string> formattedErrors;

    for (std::vector<std::string>::iterator it = _errors.begin(); it != _errors.end(); ++it) {
        std::string errorMessage = *it;

        if (_currentLine < _configLines.size()) {
            errorMessage = "Line " + Utils::StringUtils::numToString(_currentLine + 1) + ": " + errorMessage;
        }

        formattedErrors.push_back(errorMessage);
    }

    return formattedErrors;
}

static void trimWhitespace(std::string &str) {
    std::string::iterator it = str.begin();
	
    while (it != str.end() && std::isspace(*it)) {
        ++it;
    }
    str.erase(str.begin(), it);

    if (!str.empty()) {
        it = str.end();
        do {
            --it;
        } while (it != str.begin() && std::isspace(*it));
        str.erase(it + 1, str.end());
    }
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

    std::ifstream configFile(_configPath.c_str());
    if (!configFile.is_open()) {
        addError("Failed to open config file: " + _configPath);
        return;
    }

    _configLines.clear();

    std::string line;
    while (std::getline(configFile, line)) {
        std::size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        trimWhitespace(line);

        if (!line.empty()) {
            _configLines.push_back(line);
        }
    }

    if (_configLines.empty()) {
        addError("Config file is empty or only contains comments.");
    }

    configFile.close();
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
			skipWhiteSpace();
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
				skipWhiteSpace();
				line = getCurrentLine();
				if (isBlockEnd(line))
					return;
				// Parse server directives...
			}
			addError("Unterminated server block");
		}
	*/
	
    skipWhiteSpace();
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
        skipWhiteSpace();
        line = getCurrentLine();
        if (isBlockEnd(line)) {
            ++_currentLine;
            break;
        }

        if (line.find("location") == 0) {
            LocationConfig location;
            parseLocation(location);
            server.locations.push_back(location);
        }
        else if (line.find("cgi") == 0) {
            parseCGI(server);
        }
        else {
            parseDirective(line, server);
        }

        ++_currentLine;
    }

    if (!validatePaths(server)) {
        addError("Invalid server paths configuration");
    }
    if (!validatePorts(server)) {
        addError("Invalid port configuration");
    }
    if (!validateCGI(server)) {
        addError("Invalid CGI configuration");
    }
    if (!validateLocations(server)) {
        addError("Invalid location configurations");
    }
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
	
	std::string line = getCurrentLine();
    skipWhiteSpace();
    trimWhitespace(line);

    if (line.empty() || line[0] != '/') {
        addError("Invalid location path at line: " + Utils::StringUtils::numToString(_currentLine));
        return;
    }
    location.path = line;
    ++_currentLine;

    while (hasMoreLines()) {
        line = getCurrentLine();
        skipWhiteSpace();
        trimWhitespace(line);

        if (isBlockEnd(line)) {
            ++_currentLine;
            break;
        }

        if (line.find("root") == 0) {
            std::string root = line.substr(5);
            trimWhitespace(root);
            if (!root.empty()) {
                location.root = root;
            } else {
                addError("Invalid root directory at line: " + Utils::StringUtils::numToString(_currentLine));
            }
        }
        else if (line.find("index") == 0) {
            std::string index = line.substr(6);
            trimWhitespace(index);
            if (!index.empty()) {
                location.index = index;
            } else {
                addError("Invalid index file at line: " + Utils::StringUtils::numToString(_currentLine));
            }
        }
        else if (line.find("methods") == 0) {
            std::string methods = line.substr(7);
            trimWhitespace(methods);
            if (!methods.empty()) {
                std::istringstream methodStream(methods);
                std::string method;
                while (methodStream >> method) {
                    location.methods.push_back(method);
                }
            } else {
                addError("Invalid methods at line: " + Utils::StringUtils::numToString(_currentLine));
            }
        }
        else if (line.find("cgi") == 0) {
            std::string cgi = line.substr(4);
            trimWhitespace(cgi);
            if (!cgi.empty()) {
                location.cgi_path = cgi;
            } else {
                addError("Invalid CGI configuration at line: " + Utils::StringUtils::numToString(_currentLine));
            }
        }
        else if (line.find("autoindex") == 0) {
            std::string autoindex = line.substr(9);
            trimWhitespace(autoindex);
            if (autoindex == "on") {
                location.autoindex = true;
            } else if (autoindex == "off") {
                location.autoindex = false;
            } else {
                addError("Invalid autoindex value at line: " + Utils::StringUtils::numToString(_currentLine));
            }
        }
        else if (line.find("client_max_body_size") == 0) {
            std::string size = line.substr(20);
            trimWhitespace(size);
            try {
                location.client_max_body_size = std::strtoul(size.c_str(), NULL, 10);
            } catch (const std::invalid_argument&) {
                addError("Invalid client_max_body_size at line: " + Utils::StringUtils::numToString(_currentLine));
            }
        } 
        else {
            addError("Unknown directive in location block at line: " + Utils::StringUtils::numToString(_currentLine));
        }

        ++_currentLine;
    }

    if (location.path.empty() || location.path[0] != '/') {
        addError("Invalid location path: " + location.path);
    }

    for (std::vector<std::string>::const_iterator it = location.methods.begin(); it != location.methods.end(); ++it) {
        const std::string &method = *it;
        if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE") {
            addError("Invalid HTTP method: " + method + " in location path: " + location.path);
        }
    }

    if (!location.cgi_path.empty() && location.cgi_ext.empty()) {
        addError("CGI path defined but no CGI extensions provided in location path: " + location.path);
    }

    if (location.client_max_body_size == 0) {
        addError("Invalid client_max_body_size in location path: " + location.path);
    }
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
	
	while (hasMoreLines()) {
        std::string line = getCurrentLine();
        skipWhiteSpace();

        if (isBlockEnd(line)) {
            ++_currentLine;
            break;
        }

        if (line.find("cgi_ext") == 0) {
            std::string ext = line.substr(8);
            trimWhitespace(ext);
            if (!ext.empty()) {
                server.cgi_handlers[ext] = line;
            } else {
                addError("Invalid CGI extension configuration at line: " + Utils::StringUtils::numToString(_currentLine));
            }
        } 
        else if (line.find("cgi_path") == 0) {
            std::string path = line.substr(9);
            trimWhitespace(path);
            if (!path.empty()) {
                server.cgi_handlers["path"] = path;
            } else {
                addError("Invalid CGI path configuration at line: " + Utils::StringUtils::numToString(_currentLine));
            }
        }
        else if (line.find("timeout") == 0) {
            std::string timeoutStr = line.substr(8);
            trimWhitespace(timeoutStr);
            char *endPtr;
            unsigned long timeout = std::strtoul(timeoutStr.c_str(), &endPtr, 10);
            if (*endPtr == '\0' && timeout > 0) {
                server.client_timeout = timeout;
            } else {
                addError("Invalid timeout value at line: " + Utils::StringUtils::numToString(_currentLine));
            }
        }
        else {
            addError("Unknown CGI directive at line: " + Utils::StringUtils::numToString(_currentLine));
        }

        ++_currentLine;
    }

    validateCGI(server);
}

static bool validateRootDirectory(const std::string &path) {
    struct stat statBuffer;
    
    if (stat(path.c_str(), &statBuffer) != 0) {
        return false;
    }
    
    if (!S_ISDIR(statBuffer.st_mode)) {
        return false;
    }
    
    if (access(path.c_str(), R_OK) != 0) {
        return false;
    }

    return true;
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
	
    std::pair<std::string, std::string> directive = splitDirective(line);
    
    if (directive.first.empty()) {
        addError("Empty directive");
        return;
    }

    if (directive.first == "listen") {
        int port = atoi(directive.second.c_str());
        if (port <= 0 || port > 65535) {
            addError("Invalid port number for listen directive");
        } else {
            server.port = port;
        }
    }
    else if (directive.first == "server_name") {
        server.server_names.push_back(directive.second);
    }
    else if (directive.first == "root") {
        if (!validateRootDirectory(directive.second)) {
            addError("Invalid root directory: " + directive.second);
        } else {
            server.root = directive.second;
        }
    }
    else if (directive.first == "index") {
        std::istringstream stream(directive.second);
        std::string indexFile;
        while (stream >> indexFile) {
            server.index = indexFile;
        }
    }
    else if (directive.first == "error_page") {
        int errorCode = atoi(directive.second.c_str());
        if (errorCode <= 0 || errorCode > 599) {
            addError("Invalid error code for error_page directive");
        } else {
            server.error_pages[errorCode] = directive.second;
        }
    }
    else if (directive.first == "client_max_body_size") {
        char *endPtr;
        unsigned long size = std::strtoul(directive.second.c_str(), &endPtr, 10);
        if (*endPtr != '\0' || size == 0) {
            addError("Invalid client_max_body_size value");
        } else {
            server.client_max_body_size = size;
        }
    }
    else if (directive.first == "timeout") {
        char *endPtr;
        unsigned long timeout = std::strtoul(directive.second.c_str(), &endPtr, 10);
        if (*endPtr != '\0' || timeout == 0) {
            addError("Invalid timeout value");
        } else {
            server.client_timeout = timeout;
        }
    }
    else {
        addError("Unknown directive: " + directive.first);
    }
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
	
	std::string trimmedLine = line;
    trimWhitespace(trimmedLine);

    if (trimmedLine.empty()) {
        return false;
    }

    return trimmedLine[0] == '{';
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
	
	std::string trimmedLine = line;
    trimWhitespace(trimmedLine);

    if (trimmedLine.empty()) {
        return false;
    }

    return trimmedLine[trimmedLine.size() - 1] == '}';
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
	
	std::string lineTrimmed = line;
	
    size_t startPos = line.find_first_not_of(" \t");
    if (startPos == std::string::npos) {
        return std::pair<std::string, std::string>();
    }
    lineTrimmed = line.substr(startPos);

    if (lineTrimmed.empty() || lineTrimmed[0] == '#') {
        return std::pair<std::string, std::string>();
    }

    size_t position = lineTrimmed.find_first_of(" \t");

    if (position == std::string::npos) {
        return std::pair<std::string, std::string>(lineTrimmed, "");
    }

    std::string name = lineTrimmed.substr(0, position);

    size_t valueStart = lineTrimmed.find_first_not_of(" \t", position);
    if (valueStart == std::string::npos) {
        return std::pair<std::string, std::string>(name, "");
    }

    std::string value = lineTrimmed.substr(valueStart);
    value = value.substr(0, value.find_last_not_of(" \t") + 1);

    if (!value.empty() && value[0] == '"') {
        size_t endQuotePos = value.find_last_of('"');
        if (endQuotePos != std::string::npos && endQuotePos > 0) {
            value = value.substr(1, endQuotePos - 1);
        }
    }

    return std::pair<std::string, std::string>(name, value);
}

void ConfigParser::skipWhiteSpace() {
	/*
		skipWhiteSpace():
		1. WHILE hasMoreLines() AND current line is whitespace:
		   - Move to next line
		2. IF at valid line:
		   - Trim leading/trailing whitespace
		Note: Whitespace includes:
		- Empty lines
		- Lines with only spaces/tabs
		- Comment lines (starting with #)
	*/
	while (hasMoreLines()) {
        std::string &line = _configLines[_currentLine];

        trimWhitespace(line);

        if (line.empty() || line[0] == '#') {
            ++_currentLine;
            continue;
        }

        break;
    }
}

bool ConfigParser::hasMoreLines() {
	/*
		hasMoreLines():
		1. Check if current line index < total lines
		2. Return true if more lines available
		3. Return false if at end of file
		Note: This tracks position in the loaded config file
	*/
    return _currentLine < _configLines.size();
}

std::string ConfigParser::getCurrentLine() {
	/*
		getCurrentLine():
		1. IF no more lines:
		   Return empty string
		2. Return current line from loaded lines
		Note: Returns trimmed line without comments
	*/
	
	if (!hasMoreLines()) {
        return "";
    }

    std::string line = _configLines[_currentLine];

    std::size_t commentPos = line.find('#');
    if (commentPos != std::string::npos) {
        line = line.substr(0, commentPos);
    }

    trimWhitespace(line);

    return line;
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
	
	std::string formattedError;
    
    if (_currentLine > 0) {
        formattedError = "Line " + Utils::StringUtils::numToString(_currentLine) + ": ";
    }
    
    formattedError += error;
    
    if (_state != MAIN) {
        formattedError += " [Context: " + Utils::StringUtils::numToString(_state) + "]";
    }

    _errors.push_back(formattedError);
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
	
	struct stat rootStat;
    if (stat(config.root.c_str(), &rootStat) != 0 || !S_ISDIR(rootStat.st_mode)) {
        addError("Root directory does not exist or is not a directory: " + config.root);
        return false;
    }

    for (std::map<int, std::string>::const_iterator it = config.error_pages.begin(); it != config.error_pages.end(); ++it) {
        struct stat errorStat;
        if (stat(it->second.c_str(), &errorStat) != 0 || !S_ISREG(errorStat.st_mode)) {
            addError("Error page file does not exist or is not a regular file: " + it->second);
            return false;
        }
    }

    for (std::map<std::string, std::string>::const_iterator it = config.cgi_handlers.begin(); it != config.cgi_handlers.end(); ++it) {
        struct stat cgiStat;
        if (stat(it->second.c_str(), &cgiStat) != 0 || !S_ISREG(cgiStat.st_mode)) {
            addError("CGI handler does not exist or is not a regular file: " + it->second);
            return false;
        }
    }

    for (std::vector<LocationConfig>::const_iterator it = config.locations.begin(); it != config.locations.end(); ++it) {
        struct stat locationStat;
        if (stat(it->root.c_str(), &locationStat) != 0 || !S_ISDIR(locationStat.st_mode)) {
            addError("Location root directory does not exist or is not a directory: " + it->root);
            return false;
        }
    }

    return true;
}

bool ConfigParser::validatePorts(const ServerConfig &config) {
	/*
		validatePorts(config):
		1. Check port number range
		2. Check port availability
		3. Check for duplicates
		4. Return validation result
	*/
	
	std::set<int> portSet;

    if (config.port < 1 || config.port > 65535) {
        addError("Port number out of range: " + Utils::StringUtils::numToString(config.port));
        return false;
    }

    if (!portSet.insert(config.port).second) {
        addError("Duplicate port found: " + Utils::StringUtils::numToString(config.port));
        return false;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        addError("Failed to create socket for port validation.");
        return false;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        addError("Port " + Utils::StringUtils::numToString(config.port) + " is already in use.");
        return false;
    }

    close(sockfd);

    return true;
}

bool ConfigParser::validateCGI(const ServerConfig &config) {
	/*
		validateCGI(config):
		1. Check handler paths
		2. Validate extensions
		3. Check permissions
		4. Return validation result
	*/

    for (std::map<std::string, std::string>::const_iterator it = config.cgi_handlers.begin(); it != config.cgi_handlers.end(); ++it) {
        const std::string& path = it->second;

        struct stat info;
        if (stat(path.c_str(), &info) != 0) {
            addError("CGI handler path not found or accessible: " + path);
            return false;
        }

        if (!(info.st_mode & S_IXUSR)) {
            addError("CGI handler is not executable: " + path);
            return false;
        }
    }

    for (std::map<std::string, std::string>::const_iterator it = config.cgi_handlers.begin(); it != config.cgi_handlers.end(); ++it) {
        const std::string& extension = it->first;

        if (extension.empty() || extension[0] != '.') {
            addError("Invalid CGI extension format: " + extension);
            return false;
        }
    }

    for (std::map<std::string, std::string>::const_iterator it = config.cgi_handlers.begin(); it != config.cgi_handlers.end(); ++it) {
        const std::string& path = it->second;

        struct stat info;
        if (stat(path.c_str(), &info) != 0) {
            addError("CGI handler path not found or accessible: " + path);
            return false;
        }

        if (!(info.st_mode & S_IXUSR)) {
            addError("CGI handler is not executable: " + path);
            return false;
        }
    }

    return true;
}

bool ConfigParser::validateLocations(const ServerConfig &config) {
	/*
		validateLocations(config):
		1. Check for path conflicts
		2. Validate methods
		3. Check directory permissions
		4. Return validation result
	*/

    for (size_t i = 0; i < config.locations.size(); ++i) {
        for (size_t j = i + 1; j < config.locations.size(); ++j) {
            const LocationConfig &loc1 = config.locations[i];
            const LocationConfig &loc2 = config.locations[j];

            if (loc1.path != loc2.path && loc1.path.find(loc2.path) == 0) {
                addError("Path conflict between locations: " + loc1.path + " and " + loc2.path);
                return false;
            }
        }
    }

    for (std::vector<LocationConfig>::const_iterator locIt = config.locations.begin(); locIt != config.locations.end(); ++locIt) {
        const LocationConfig &location = *locIt;
        for (std::vector<std::string>::const_iterator methodIt = location.methods.begin(); methodIt != location.methods.end(); ++methodIt) {
            const std::string &method = *methodIt;
            if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE" && method != "PATCH" && method != "HEAD") {
                addError("Invalid HTTP method in location " + location.path + ": " + method);
                return false;
            }
        }
    }

    for (std::vector<LocationConfig>::const_iterator locIt = config.locations.begin(); locIt != config.locations.end(); ++locIt) {
        const LocationConfig &location = *locIt;
        struct stat info;
        if (stat(location.root.c_str(), &info) != 0) {
            addError("Directory not found or accessible for location " + location.path + ": " + location.root);
            return false;
        }

        if (!(info.st_mode & S_IRUSR) || !(info.st_mode & S_IXUSR)) {
            addError("Insufficient permissions to read/execute directory for location " + location.path + ": " + location.root);
            return false;
        }
    }

    return true;
}
