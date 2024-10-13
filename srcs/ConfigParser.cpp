/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:26:31 by sehosaf           #+#    #+#             */
/*   Updated: 2024/10/28 14:52:02 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

ConfigParser::ConfigParser(const std::string &filename) : _filename(filename) {}

bool ConfigParser::parse() {
	std::ifstream file(_filename.c_str());
	if (!file.is_open()) {
		std::cerr << "Failed to open configuration file: " << _filename << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string key, value;
		if (std::getline(iss, key, '=') && std::getline(iss, value)) {
			_config[key] = value;
		}
	}
	return true;
}

std::map<std::string, std::string> ConfigParser::getConfig() const { return _config; }
