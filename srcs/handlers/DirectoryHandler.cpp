/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryHandler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:07:50 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 22:14:57 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "DirectoryHandler.hpp"
#include "FileHandler.hpp"
#include "../utils/Utils.hpp"
#include <sys/stat.h>
#include <ctime>
#include <sstream>

using namespace Utils;

Response DirectoryHandler::handleDirectory(const std::string &path, const LocationConfig &loc) {
	if (!loc.index.empty()) {
		std::string indexPath = path + "/" + loc.index;
		struct stat st;
		if (stat(indexPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
			return FileHandler::serveFile(indexPath, "/index.html");
	}

	if (!loc.autoindex)
		return Response(403, "Forbidden");

	std::string listing = createListing(path, loc.path);
	if (listing.empty())
		return Response(500, "Internal Server Error");

	Response response(200);
	response.addHeader("Content-Type", "text/html");
	response.setBody(listing);
	return response;
}

std::string DirectoryHandler::createListing(const std::string &path, const std::string &urlPath) {
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return "";

	std::stringstream html;
	html << createListingHeader(urlPath)
		 << createListingBody(dir, path, urlPath)
		 << "</table></body></html>";

	closedir(dir);
	return html.str();
}

std::string DirectoryHandler::createListingHeader(const std::string &urlPath) {
	std::stringstream header;
	header << "<html><head>"
		   << "<title>Directory: " << urlPath << "</title>"
		   << "<style>"
		   << "table{border-collapse:collapse;width:100%}"
		   << "th,td{padding:8px;text-align:left}"
		   << "th{background:#f2f2f2}"
		   << ".delete-btn{color:red;cursor:pointer;margin-left:10px}"
		   << "</style>"
		   << createDeleteScript()
		   << "</head><body>"
		   << "<h1>Directory: " << urlPath << "</h1>"
		   << "<table><tr><th>Name</th><th>Size</th>"
		   << "<th>Last Modified</th><th>Actions</th></tr>";
	return header.str();
}

std::string DirectoryHandler::createDeleteScript() {
	return "<script>"
		   "function deleteFile(path) {"
		   "  if(confirm('Are you sure you want to delete this file?')) {"
		   "    fetch(path, {method:'DELETE'})"
		   "    .then(response => {"
		   "      if(response.ok) {"
		   "        alert('File deleted successfully');"
		   "        window.location.reload();"
		   "      } else {"
		   "        response.text().then(text => alert('Delete failed: ' + text));"
		   "      }"
		   "    })"
		   "    .catch(err => alert('Delete failed: ' + err));"
		   "  }"
		   "}</script>";
}

std::string DirectoryHandler::createListingBody(DIR* dir, const std::string &path, const std::string &urlPath) {
	std::stringstream body;
	struct dirent* entry;

	if (urlPath != "/")
		body << "<tr><td><a href=\"../\">..</a></td><td>-</td><td>-</td><td></td></tr>";

	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;

		struct stat st;
		std::string fullPath = path + "/" + name;
		if (stat(fullPath.c_str(), &st) == 0) {
			bool needsSlash = !urlPath.empty() && urlPath[urlPath.length() - 1] != '/';

			body << "<tr><td><a href=\"" << urlPath
				 << (needsSlash ? "/" : "")
				 << FileHandler::urlDecode(name)
				 << (S_ISDIR(st.st_mode) ? "/" : "")
				 << "\">" << name << "</a></td>"
				 << "<td>" << formatFileSize(st) << "</td>"
				 << "<td>" << formatModTime(st) << "</td>"
				 << "<td>";

			if (!S_ISDIR(st.st_mode)) {
				body << "<a class='delete-btn' onclick='deleteFile(\""
					 << urlPath
					 << (needsSlash ? "/" : "")
					 << FileHandler::urlDecode(name) << "\")'>Delete</a>";
			}

			body << "</td></tr>";
		}
	}
	return body.str();
}

std::string DirectoryHandler::formatFileSize(const struct stat &st) {
	if (S_ISDIR(st.st_mode))
		return "-";
	return StringUtils::numToString(st.st_size) + " bytes";
}

std::string DirectoryHandler::formatModTime(const struct stat &st) {
	char timebuf[32];
	strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
			 localtime(&st.st_mtime));
	return std::string(timebuf);
}
