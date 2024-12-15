/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TempFileManager.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/14 12:34:36 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/14 12:35:00 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TEMPFILEMANAGER_HPP
#define TEMPFILEMANAGER_HPP

#include <string>
#include <sys/stat.h>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>

class TempFileManager {
	private:

		static bool ensureTempDirExists() {
			struct stat st;
			const std::string& dir = getTempDir();
			if (stat(dir.c_str(), &st) == 0) {
				return true;
			}
			return mkdir(dir.c_str(), 0755) == 0;
		}

	public:
		static std::string createTempFile() {
			if (!ensureTempDirExists()) {
				return "";
			}

			std::string templatePath = getTempDir() + "/webserv_XXXXXX";
			char* tempPath = new char[templatePath.length() + 1];
			strcpy(tempPath, templatePath.c_str());

			int fd = mkstemp(tempPath);
			std::string result;

			if (fd != -1) {
				result = tempPath;
				close(fd);
			}

			delete[] tempPath;
			return result;
		}

		static int openTempFileForReading(const std::string& path) {
			return open(path.c_str(), O_RDONLY);
		}

		static void deleteTempFile(const std::string& path) {
			unlink(path.c_str());
		}

		static const std::string& getTempDir();
};

#endif //TEMPFILEMANAGER_HPP
