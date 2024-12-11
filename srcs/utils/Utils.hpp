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
#include <sys/stat.h>
#include <ctime>

class Utils {
	public:
		// File operations
		static bool fileExists(const std::string& path);
		static bool isDirectory(const std::string& path);
		static bool isReadable(const std::string& path);
		static std::string getFileExtension(const std::string& path);
		static size_t getFileSize(const std::string& path);
		static time_t getLastModified(const std::string& path);

		// String operations
		static std::string trim(const std::string& str);
		static std::string numToString(int value);
		static std::string numToString(size_t value);
		static std::string numToString(long value);

		// Path operations
		static std::string joinPath(const std::string& base, const std::string& path);
		static bool isSubPath(const std::string& base, const std::string& path);

		// Time operations
		static std::string formatTime(time_t time);
		static std::string getHTTPDate();

		// System operations
		static bool setNonBlocking(int fd);
		static bool setReuseAddr(int fd);
		static std::string getErrorString(int errnum);
};

#endif
