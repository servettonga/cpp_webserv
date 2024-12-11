/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:26:31 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/07 18:39:24 by jdepka           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include "../server/ServerConfig.hpp"

class ConfigParser {
	public:
		explicit ConfigParser(const std::string &configPath);
		~ConfigParser();

		// Main interface
		std::vector<ServerConfig> parse();
		void reload();
		bool validate();
		std::vector<std::string> getErrors() const;

	private:
		// File state
		std::string _configPath;
		std::vector<std::string> _configLines;
		size_t _currentLine;
		std::vector<std::string> _errors;

		// Core parsing methods
		void readConfigFile();
		void parseServer(ServerConfig &server);
		void parseLocation(LocationConfig &location);
		void parseCGI(ServerConfig &server);
		void parseDirective(const std::string &line, ServerConfig &server);

		// Helper methods
		bool isBlockStart(const std::string &line) const;
		bool isBlockEnd(const std::string &line) const;
		std::pair<std::string, std::string> splitDirective(const std::string &line) const;
		void skipWhitespace();
		bool hasMoreLines() const;
		std::string getCurrentLine() const;
		void addError(const std::string &error);
		std::string trimString(const std::string &str) const;
		void parseServerNames(const std::string &value, ServerConfig &server);
		void parseErrorPage(const std::string &value, ServerConfig &server);
		void parseAllowedMethods(const std::string &value, LocationConfig &location);
		unsigned long parseSize(const std::string &value);

		// Validation methods
		bool validatePaths(const ServerConfig &config) const;
		bool validatePorts(const ServerConfig &config) const;
		bool validateCGI(const ServerConfig &config) const;
		bool validateLocations(const ServerConfig &config) const;

		// Prevent copying
		ConfigParser(const ConfigParser&);
		ConfigParser &operator=(const ConfigParser&);
};

#endif
