/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryHandler.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:07:50 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:25:29 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DIRECTORY_HANDLER_HPP
#define DIRECTORY_HANDLER_HPP

#include "../WebServ.hpp"
#include "../http/Response.hpp"
#include "../config/ServerConfig.hpp"

class DirectoryHandler {
	public:
		static Response handleDirectory(const std::string &dirPath, const LocationConfig &location, const std::string &requestPath, const ServerConfig *config);
		static std::string createListing(const std::string &path, const std::string &urlPath);

	private:
		static std::string createListingHeader(const std::string &urlPath);
		static std::string createDeleteScript();
		static std::string formatFileSize(const struct stat &st);
		static std::string formatModTime(const struct stat &st);
		static std::string createListingBody(DIR *dir, const std::string &path, const std::string &urlPath);
};

#endif
