/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:25:04 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/26 17:37:21 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils/ConfigParser.hpp"
#include "server/Server.hpp"
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
		ServerConfig config;
		// Configure server
		config.host = "0.0.0.0";
		config.port = 8080;
		config.root = "www";

		// root location
		LocationConfig rootLoc;
		rootLoc.path = "/";
		rootLoc.root = "www";
		rootLoc.index = "index.html";
		rootLoc.autoindex = false;
		rootLoc.methods.push_back("GET");
		config.locations.push_back(rootLoc);

		// /files location
		LocationConfig filesLoc;
		filesLoc.path = "/files";
		filesLoc.root = "www";
		filesLoc.autoindex = true;
		filesLoc.methods.push_back("GET");
		config.locations.push_back(filesLoc);

		Server server(config);
		server.start();
	} catch (const std::exception &e) {
		std::cerr << "Server error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
