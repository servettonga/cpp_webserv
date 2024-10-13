/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:26:31 by sehosaf           #+#    #+#             */
/*   Updated: 2024/10/27 19:25:21 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <map>

class ConfigParser {
	public:
		explicit ConfigParser(const std::string &filename);
		bool parse();
		std::map<std::string, std::string> getConfig() const;

	private:
		std::string _filename;
		std::map<std::string, std::string> _config;
};

#endif // CONFIGPARSER_HPP