/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:26:31 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 19:18:58 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <vector>
#include <string>
#include <map>

// Configuration structures to store the configuration data for servers and locations
struct LocationConfig {
	std::string path;
	std::vector<std::string> allowedMethods;
	std::string root;
	bool autoindex;
	std::string index;
	std::string redirect;
	size_t clientMaxBodySize;
	std::map<std::string, std::string> cgiExtensions;
	std::string uploadPath;
};

struct ServerConfig {
	std::string host;
	int port;
	std::string serverName;
	std::map<int, std::string> errorPages;
	size_t clientMaxBodySize;
	std::vector<LocationConfig> locations;
};

// The ConfigParser class reads the configuration file and populates the configuration structures
class ConfigParser {
	public:
		explicit ConfigParser(const std::string& configFilePath);
		~ConfigParser();

		const std::vector<ServerConfig>& getServerConfigs() const;

	private:
		std::vector<ServerConfig> serverConfigs;

		// Private methods for parsing
		void parseConfigFile(const std::string& configFilePath);
		void parseServerBlock();
		void parseLocationBlock(ServerConfig& serverConfig);

		// Helper methods
		void skipWhitespaceAndComments();
		std::string getNextToken();
		void expectToken(const std::string& expected);

		// Configuration file content
		std::string configContent;
		size_t currentPosition;

		// Error handling
		void throwError(const std::string& message);
};

#endif // CONFIGPARSER_HPP
