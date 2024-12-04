/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:25:04 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/04 17:06:26 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils/ConfigParser.hpp"
#include "server/ServerGroup.hpp"
#include <iostream>

int main(const int argc, char *argv[]) {
	try {
		std::string configFile = argc > 1 ? argv[1] : "config/default.conf";
		if (argc == 2) {
			configFile = argv[1];
		} else if (argc > 2) {
			std::cerr << "Usage: " << argv[0] << " [configuration file]\n";
			return 1;
		}
		if (configFile.find(".conf") == std::string::npos) {
			std::cerr << "Invalid configuration file.\n";
			return 1;
		}
		(void)configFile; // TODO: use config file
		std::cout << "Using hardcoded configuration (config parsing not implemented yet)\n";

		// First server config (main)
		ServerConfig config;
		config.host = "0.0.0.0";
		config.port = 8080;
		config.root = "www";

		// Root location for the main server
		LocationConfig rootLoc;
		rootLoc.path = "/";
		rootLoc.root = "www";
		rootLoc.index = "index.html";
		rootLoc.autoindex = false;
		rootLoc.methods.push_back("GET");
		config.locations.push_back(rootLoc);

		// Files location for the main server
		LocationConfig filesLoc;
		filesLoc.path = "/files";
		filesLoc.root = "www";
		filesLoc.autoindex = true;
		filesLoc.methods.push_back("GET");
		config.locations.push_back(filesLoc);

		// Uploads location for the main server
		LocationConfig uploadLoc;
		uploadLoc.path = "/files/uploads";
		uploadLoc.root = "www";
		uploadLoc.autoindex = true;
		uploadLoc.client_max_body_size = 10 * 1024 * 1024; // 10MB
		uploadLoc.methods.push_back("GET");
		uploadLoc.methods.push_back("POST");
		uploadLoc.methods.push_back("DELETE");
		config.locations.push_back(uploadLoc);

		// Second server config (portfolio)
		ServerConfig config2;
		config2.host = "portfolio.localhost";
		config2.port = 8081;
		config2.root = "www";

		// Root location for portfolio server
		LocationConfig rootLoc2;
		rootLoc2.path = "/";
		rootLoc2.root = "www/portfolio";
		rootLoc2.index = "index.html";
		rootLoc2.autoindex = false;
		rootLoc2.methods.push_back("GET");
		config2.locations.push_back(rootLoc2);

		ServerGroup serverGroup;
		serverGroup.addServer(config);
		serverGroup.addServer(config2);
		serverGroup.start();
	} catch (const std::exception &e) {
		std::cerr << "Server error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
