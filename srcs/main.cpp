/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:25:04 by sehosaf           #+#    #+#             */
/*   Updated: 2024/10/27 19:22:50 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "Server.hpp"
#include <iostream>

int main(const int argc, char *argv[]) {
	const std::string config_file = argc > 1 ? argv[1] : "default.conf";	// default config file
	if (argc > 2) {
		std::cerr << "Usage: " << argv[0] << " [config_file]\n";
		return 1;
	}
	if (config_file.find(".conf") == std::string::npos) {	// check if the file is a .conf file
		std::cerr << "Invalid configuration file.\n";
		return 1;
	}
	ConfigParser parser(config_file);
	if (!parser.parse()) {
		std::cerr << "Failed to parse configuration file.\n";
		return 1;
	}
	Server server(parser.getConfig());
	if (!server.initialize()) {
		std::cerr << "Failed to initialize server.\n";
		return 1;
	}
	server.run();
	return 0;
}
