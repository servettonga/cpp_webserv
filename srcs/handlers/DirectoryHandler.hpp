/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryHandler.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:07:50 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 22:13:55 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DIRECTORY_HANDLER_HPP
#define DIRECTORY_HANDLER_HPP

#include "../http/Response.hpp"
#include "../server/ServerConfig.hpp"
#include <string>
#include <sys/stat.h>
#include <dirent.h>


class DirectoryHandler {
	public:
		static Response handleDirectory(const std::string& dirPath, const LocationConfig& location, const std::string& requestPath);
		static std::string createListing(const std::string& path, const std::string& urlPath);

	private:
		static std::string createListingHeader(const std::string& urlPath);
		static std::string createDeleteScript();

		static std::string formatFileSize(const struct stat &st);

		static std::string formatModTime(const struct stat &st);

		static std::string createListingBody(DIR *dir, const std::string &path, const std::string &urlPath);

};

#endif
