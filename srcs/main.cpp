/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:25:04 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/10 13:33:25 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils/ConfigParser.hpp"
#include "server/ServerGroup.hpp"
#include <iostream>
#include <csignal>

void displayErrors(const std::vector<std::string>& errors) {
	for (std::vector<std::string>::const_iterator it = errors.begin();
		 it != errors.end(); ++it) {
		std::cerr << "Config Error: " << *it << std::endl;
	}
}

int main(const int argc, char *argv[]) {
	try {
		std::string configFile = argc > 1 ? argv[1] : "config/default.conf";

		std::cout << "Starting server with config file: " << configFile << std::endl;

		// Create config parser and validate configuration
		ConfigParser parser(configFile);
		std::cout << "Validating configuration..." << std::endl;

		if (!parser.validate()) {
			std::cerr << "Configuration validation failed:\n";
			displayErrors(parser.getErrors());
			return 1;
		}

		// Parse configuration
		std::cout << "Parsing configuration..." << std::endl;
		std::vector<ServerConfig> configs = parser.parse();

		if (configs.empty()) {
			std::cerr << "No valid server configurations found\n";
			return 1;
		}

		std::cout << "Initializing server group..." << std::endl;
		ServerGroup serverGroup(configFile);

		std::cout << "Adding servers to group..." << std::endl;
		for (std::vector<ServerConfig>::iterator it = configs.begin();
			 it != configs.end(); ++it) {
			serverGroup.addServer(*it);
		}

		std::cout << "Setting up signal handlers..." << std::endl;
		signal(SIGPIPE, SIG_IGN);

		std::cout << "Starting server group..." << std::endl;
		serverGroup.start();

	} catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
