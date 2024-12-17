/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 22:50:59 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/24 10:10:40 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"
#include <fcntl.h>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <algorithm>

bool Utils::fileExists(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

bool Utils::isDirectory(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool Utils::isReadable(const std::string& path) {
	return access(path.c_str(), R_OK) == 0;
}

std::string Utils::getFileExtension(const std::string& path) {
	size_t pos = path.find_last_of('.');
	return pos != std::string::npos ? path.substr(pos + 1) : "";
}

size_t Utils::getFileSize(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0 ? st.st_size : 0;
}

time_t Utils::getLastModified(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0 ? st.st_mtime : 0;
}

std::string Utils::trim(const std::string& str) {
	if (str.empty()) return str;

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

std::string Utils::joinPath(const std::string& base, const std::string& path) {
	if (base.empty()) return path;
	if (path.empty()) return base;

	bool baseHasSlash = base[base.length() - 1] == '/';
	bool pathHasSlash = path[0] == '/';

	if (baseHasSlash && pathHasSlash)
		return base + path.substr(1);
	if (!baseHasSlash && !pathHasSlash)
		return base + '/' + path;
	return base + path;
}

bool Utils::isSubPath(const std::string& base, const std::string& path) {
	return path.find(base) == 0;
}

std::string Utils::formatTime(time_t time) {
	char buf[32];
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&time));
	return std::string(buf);
}

std::string Utils::getHTTPDate() {
	time_t now = time(NULL);
	char buf[32];
	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
	return std::string(buf);
}

bool Utils::setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return false;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

bool Utils::setReuseAddr(int fd) {
	int opt = 1;
	return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) >= 0;
}

std::string Utils::getErrorString(int errnum) {
	return std::string(strerror(errnum));
}

const std::string Utils::toUpper(const std::string string) {
	std::string upperString = string;
	std::transform(upperString.begin(), upperString.end(), upperString.begin(), ::toupper);
	return upperString.c_str();
}
