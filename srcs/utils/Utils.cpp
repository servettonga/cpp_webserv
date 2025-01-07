/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 22:50:59 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 23:05:33 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>

std::string Utils::trim(const std::string &str) {
	if (str.empty())
		return str;

	size_t start = 0;
	size_t end = str.length();

	while (start < end && isspace(str[start])) ++start;
	while (end > start && isspace(str[end - 1])) --end;

	return str.substr(start, end - start);
}

std::string Utils::numToString(int value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

std::string Utils::numToString(size_t value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

std::string Utils::numToString(long value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

std::string Utils::numToString(off_t value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

const std::string Utils::toUpper(const std::string string) {
	std::string upperString = string;
	std::transform(upperString.begin(), upperString.end(), upperString.begin(), ::toupper);
	return upperString.c_str();
}

int Utils::stringToNum(std::basic_string<char> &basicString) {
	int num;
	std::istringstream(basicString) >> num;
	return num;
}
