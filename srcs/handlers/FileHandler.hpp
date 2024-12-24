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

#include "../WebServ.hpp"
#include "../http/Response.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/Request.hpp"

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
		static bool saveUploadedFile(const std::string &filepath, const std::string &content);
		static bool isValidFilePath(const std::string &path);
		static std::string sanitizeFilename(const std::string &filename);
		static std::string getType(const std::string &path);
	public:
		static Response serveFile(const std::string &path, const std::string &urlPath);
		static Response handleFileUpload(const Request &request, const LocationConfig &loc);
		static Response handleFileDelete(const Request &request, const LocationConfig &loc);

		static std::string constructFilePath(const std::string &uri, const LocationConfig &loc);
		static std::string urlDecode(const std::string &encoded);
};

#endif
