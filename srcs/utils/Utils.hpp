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
#include <vector>
#include <map>
#include <ctime>

namespace Utils {
	// String manipulation
	class StringUtils {
		public:
			static std::vector<std::string> split(const std::string& str, char delim);
			static std::string trim(const std::string& str);
			static std::string toLower(const std::string& str);
			static std::string toUpper(const std::string& str);
			static bool startsWith(const std::string& str, const std::string& prefix);
			static bool endsWith(const std::string& str, const std::string& suffix);
			static std::string replace(const std::string& str,
									   const std::string& from,
									   const std::string& to);
			static std::string numToString(int value);
			static std::string numToString(long value);
			static std::string numToString(unsigned long value);
			static std::string numToString(long long value);
	};

	// File operations
	class FileUtils {
		public:
			static bool fileExists(const std::string& path);
			static bool isDirectory(const std::string& path);
			static bool isReadable(const std::string& path);
			static bool isWritable(const std::string& path);
			static bool isExecutable(const std::string& path);
			static std::string getFileExtension(const std::string& path);
			static size_t getFileSize(const std::string& path);
			static time_t getLastModified(const std::string& path);
			static std::vector<std::string> listDirectory(const std::string& path);
	};

	// Time utilities
	class TimeUtils {
		public:
			static std::string getCurrentTime();
			static std::string formatTime(time_t time);
			static std::string getHTTPDate();
			static time_t parseHTTPDate(const std::string& date);
	};

	// Path manipulation
	class PathUtils {
		public:
			static std::string normalizePath(const std::string& path);
			static std::string joinPath(const std::string& base, const std::string& path);
			static bool isSubPath(const std::string& base, const std::string& path);
			static std::string getAbsolutePath(const std::string& path);
	};

	// HTTP utilities
	class HTTPUtils {
		public:
			static std::string urlEncode(const std::string& str);
			static std::string urlDecode(const std::string& str);
			static std::map<std::string, std::string> parseQueryString(const std::string& query);
			static std::string getMimeType(const std::string &ext);
	};

	// System utilities
	class SystemUtils {
		public:
			static int getProcessId();
			static std::string getHostname();
			static bool setNonBlocking(int fd);
			static bool setReuseAddr(int fd);
			static std::string getErrorString(int errnum);
	};
}

#endif
