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

#include "WebServ.hpp"
#include "config/ConfigParser.hpp"
#include "server/ServerGroup.hpp"

void displayErrors(const std::vector<std::string>& errors) {
	for (std::vector<std::string>::const_iterator it = errors.begin();
		 it != errors.end(); ++it) {
		std::cerr << "Config Error: " << *it << "\n";
	}
}

int main(const int argc, char *argv[]) {
	try {
		std::string configFile = argc > 1 ? argv[1] : DEFAULT_CONFIG_FILE;

		std::cout << GREEN << "Starting server with config file: " << configFile << "\n" << RESET;

		// Create config parser and validate configuration
		ConfigParser parser(configFile);
		std::cout << "Validating configuration...\n";

		if (!parser.validate()) {
			std::cerr << RED << "Configuration validation failed:\n" << RESET;
			displayErrors(parser.getErrors());
			return 1;
		}

		// Parse configuration
		std::cout << "Parsing configuration..." << std::endl;
		std::vector<ServerConfig> configs = parser.parse();

		if (configs.empty()) {
			std::cerr << RED << "No valid server configurations found\n" << RESET;
			return 1;
		}

		std::cout << "Initializing server group...\n";
		ServerGroup serverGroup(configFile);
		for (std::vector<ServerConfig>::iterator it = configs.begin();
			 it != configs.end(); ++it) {
			serverGroup.addServer(*it);
		}

		std::cout << "Setting up signal handlers...\n";
		signal(SIGPIPE, SIG_IGN);

		std::cout << "Starting server group...\n";
		serverGroup.start();

	} catch (const std::exception& e) {
		std::cerr << RED << "Fatal error: " << e.what() << std::endl << RESET;
		return 1;
	}

	return 0;
}
