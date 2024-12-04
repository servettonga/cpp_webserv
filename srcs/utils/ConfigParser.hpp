/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:26:31 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/04 17:43:16 by jdepka           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include "../server/ServerConfig.hpp"
#include <fstream>
#include <sys/stat.h>
#include <set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>

class ConfigParser {
	public:
		// Constructors and Destructor
		explicit ConfigParser(const std::string &configPath);
		~ConfigParser();

		// Main parsing methods
		std::vector<ServerConfig> parse();
		void reload();

		// Validation methods
		bool validate() ;
		std::vector<std::string> getErrors() ;

	public:
		// File handling
		std::string _configPath;
		std::vector<std::string> _configLines;
		size_t _currentLine;
		std::vector<std::string> _errors;

		// Parsing state
		enum ParserState {
			MAIN,
			SERVER,
			LOCATION,
			CGI
		};
		ParserState _state;

		// Parsing methods
		void readConfigFile();
		void parseServer(ServerConfig &server);
		void parseLocation(LocationConfig &location);
		void parseCGI(ServerConfig &server);
		void parseDirective(const std::string &line, ServerConfig &server);

		// Helper methods
		bool isBlockStart(const std::string &line) ;
		bool isBlockEnd(const std::string &line) ;
		std::pair<std::string, std::string> splitDirective(const std::string &line);
		void skipWhiteSpace();
		bool hasMoreLines() ;
		std::string getCurrentLine();
		void addError(const std::string &error);

		// Validation methods
		bool validatePaths(const ServerConfig &config) ;
		bool validatePorts(const ServerConfig &config) ;
		bool validateCGI(const ServerConfig &config) ;
		bool validateLocations(const ServerConfig &config) ;
};

#endif
