/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:26:31 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/04 21:23:52 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include "../server/ServerConfig.hpp"

class ConfigParser {
	public:
		// Constructors and Destructor
		explicit ConfigParser(const std::string &configPath);
		~ConfigParser();

		// Main parsing methods
		std::vector<ServerConfig> parse();
		void reload();

		// Validation methods
		static bool validate() ;
		static std::vector<std::string> getErrors() ;

	private:
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
		static bool isBlockStart(const std::string &line) ;
		static bool isBlockEnd(const std::string &line) ;
		std::pair<std::string, std::string> splitDirective(const std::string &line);
		void skipWhitespace();
		static bool hasMoreLines() ;
		std::string getCurrentLine();
		void addError(const std::string &error);

		// Validation methods
		static bool validatePaths(const ServerConfig &config) ;
		static bool validatePorts(const ServerConfig &config) ;
		static bool validateCGI(const ServerConfig &config) ;
		static bool validateLocations(const ServerConfig &config) ;
};

#endif
