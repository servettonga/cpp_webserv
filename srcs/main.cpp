/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:25:04 by sehosaf           #+#    #+#             */
/*   Updated: 2024/10/31 22:08:01 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "Server.hpp"
#include <iostream>

int main(const int argc, char *argv[]) {
	try {
		std::string configFile = argc > 1 ? argv[1] : "default.conf";
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
		Server server(configFile);
		server.run();
	} catch (const std::exception& e) {
		std::cerr << "Server error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
