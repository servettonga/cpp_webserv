/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 22:50:59 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/09 19:25:33 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

class Utils {
	public:
		// String operations
		static std::string trim(const std::string& str);
		static std::string numToString(int value);
		static std::string numToString(size_t value);
		static std::string numToString(long value);
		static const std::string toUpper(const std::string string);
		static int stringToNum(std::basic_string<char> &basicString);
};

#endif
