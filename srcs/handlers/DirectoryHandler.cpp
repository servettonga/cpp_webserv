/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryHandler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:07:50 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/03 12:06:03 by sehosaf          ###   ########.fr       */
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
			return FileHandler::serveFile(indexPath, "/" + loc.index);
	}
	if (!loc.autoindex)
		return Response::makeErrorResponse(403);

	// Get the relative path from root
	std::string relativePath;
	if (path.find(loc.root) == 0) {
		relativePath = path.substr(loc.root.length());
		while (!relativePath.empty() && relativePath[0] == '/')
			relativePath = relativePath.substr(1);
	}

	// Use the complete request path for URLs
	std::string urlPath = relativePath.empty() ? "/" : "/" + relativePath;

	std::string listing = createListing(path, urlPath);
	if (listing.empty())
		return Response::makeErrorResponse(500);

	Response response(200);
	response.addHeader("Content-Type", "text/html");
	response.setBody(listing);
	return response;
}

std::string DirectoryHandler::createListing(const std::string &path, const std::string &urlPath) {
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return "";

	// Get the full URL path for display and links
	std::string displayPath = urlPath;
	if (displayPath.empty() || displayPath[0] != '/')
		displayPath = "/" + displayPath;
	if (displayPath[displayPath.length() - 1] == '/')
		displayPath = displayPath.substr(0, displayPath.length() - 1);

	std::stringstream html;
	html << createListingHeader(displayPath)
		 << createListingBody(dir, path, displayPath)
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

	// Add the parent directory link
	if (urlPath != "/" && !urlPath.empty()) {
		std::string parentPath = urlPath.substr(0, urlPath.find_last_of('/'));
		if (parentPath.empty()) parentPath = "/";
		body << "<tr><td><a href=\"" << parentPath << "\">..</a></td>"
			 << "<td>-</td><td>-</td><td></td></tr>";
	}

	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;

		struct stat st;
		std::string fullPath = path + "/" + name;
		if (stat(fullPath.c_str(), &st) == 0) {
			// Construct the proper URL path for links
			std::string entryUrlPath = urlPath + "/" + FileHandler::urlDecode(name);

			body << "<tr><td><a href=\"" << entryUrlPath
				 << (S_ISDIR(st.st_mode) ? "/" : "")
				 << "\">" << name << "</a></td>"
				 << "<td>" << formatFileSize(st) << "</td>"
				 << "<td>" << formatModTime(st) << "</td>"
				 << "<td>";

			if (!S_ISDIR(st.st_mode)) {
				body << "<a class='delete-btn' onclick='deleteFile(\""
					 << entryUrlPath << "\")'>Delete</a>";
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
