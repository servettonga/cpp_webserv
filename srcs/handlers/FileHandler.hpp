/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:09:27 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 22:30:09 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include "../http/Response.hpp"
#include "../server/ServerConfig.hpp"
#include "../http/HTTPRequest.hpp"
#include <string>

class FileHandler {
	private:
		struct FileData {
			std::string filename;
			std::string content;
			bool isValid;
			FileData() : isValid(false) {}
		};

		static std::string extractBoundary(const std::string &contentType);
		static FileData parseMultipartData(const std::string &body, const std::string &boundary);
		static std::string constructUploadPath(const LocationConfig &loc, const std::string &filename);
		static bool saveUploadedFile(const std::string &filepath, const std::string &content);
		static Response createSuccessResponse();
	public:
		static Response serveFile(const std::string &path, const std::string &urlPath);
		static Response handleFileUpload(const HTTPRequest &request, const LocationConfig &loc);
		static Response handleFileDelete(const HTTPRequest &request, const LocationConfig &loc);

		static std::string constructFilePath(const std::string &uri, const LocationConfig &loc);
		static bool isValidFilePath(const std::string &path);
		static std::string sanitizeFilename(const std::string &filename);
		static bool validatePath(const std::string &path);
		static std::string urlDecode(const std::string &encoded);
		static std::string getType(const std::string &path);
};

#endif
