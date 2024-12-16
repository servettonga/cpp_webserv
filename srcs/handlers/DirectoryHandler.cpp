/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryHandler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:07:50 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/12 18:23:40 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "DirectoryHandler.hpp"
#include "FileHandler.hpp"
#include "../utils/Utils.hpp"
#include <sys/stat.h>
#include <ctime>
#include <sstream>

Response DirectoryHandler::handleDirectory(const std::string &dirPath, const LocationConfig &location, const std::string& requestPath) {
	if (!location.autoindex)
		return Response::makeErrorResponse(403);

	DIR *dir = opendir(dirPath.c_str());
	if (!dir)
		return Response::makeErrorResponse(404);
	closedir(dir);

	// Use the actual request path for the directory listing
	std::string html = createListing(dirPath, requestPath);
	if (html.empty())
		return Response::makeErrorResponse(500);

	Response response(200);
	response.addHeader("Content-Type", "text/html");
	response.setBody(html);
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
	html << createListingHeader(displayPath);
	html << createListingBody(dir, path, displayPath);
	html << "</table></body></html>";

	closedir(dir);
	return html.str();
}

std::string DirectoryHandler::createListingHeader(const std::string &urlPath) {
	std::string displayPath = urlPath;
	if (!displayPath.empty() && displayPath[displayPath.length() - 1] == '/')
		displayPath = displayPath.substr(0, displayPath.length() - 1);

	std::stringstream header;
	header << "<!DOCTYPE html>\n"
		   << "<html lang=\"en\">\n"
		   << "<head>\n"
		   << "    <meta charset=\"UTF-8\">\n"
		   << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
		   << "    <title>Directory: " << displayPath << "</title>\n"
		   << "    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css\" rel=\"stylesheet\">\n"
		   << "    <style>\n"
		   << "        :root {\n"
		   << "            --bg-color: #ffffff;\n"
		   << "            --text-color: #212529;\n"
		   << "            --card-bg: #ffffff;\n"
		   << "            --border-color: #dee2e6;\n"
		   << "            --primary-color: #0d6efd;\n"
		   << "            --hover-bg: rgba(13, 110, 253, 0.05);\n"
		   << "        }\n"
		   << "        body {\n"
		   << "            background-color: var(--bg-color);\n"
		   << "            color: var(--text-color);\n"
		   << "        }\n"
		   << "        .card {\n"
		   << "            background-color: var(--card-bg);\n"
		   << "            border: 1px solid var(--border-color);\n"
		   << "            border-radius: 8px;\n"
		   << "            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);\n"
		   << "        }\n"
		   << "        .table {\n"
		   << "            margin-bottom: 0;\n"
		   << "        }\n"
		   << "        .delete-btn {\n"
		   << "            color: #dc3545;\n"
		   << "            cursor: pointer;\n"
		   << "            padding: 0.25rem 0.5rem;\n"
		   << "            border-radius: 0.25rem;\n"
		   << "        }\n"
		   << "        .delete-btn:hover {\n"
		   << "            background-color: rgba(220, 53, 69, 0.1);\n"
		   << "        }\n"
		   << "    </style>\n"
		   << createDeleteScript()
		   << "</head>\n"
		   << "<body>\n"
		   << "    <nav class=\"navbar navbar-expand-lg mb-4\">\n"
		   << "        <div class=\"container\">\n"
		   << "            <a class=\"navbar-brand\" href=\"/\">Webserv</a>\n"
		   << "            <div class=\"collapse navbar-collapse\">\n"
		   << "                <ul class=\"navbar-nav me-auto\">\n"
		   << "                    <li class=\"nav-item\">\n"
		   << "                        <a class=\"nav-link\" href=\"/static\">Static Files</a>\n"
		   << "                    </li>\n"
		   << "                    <li class=\"nav-item\">\n"
		   << "                        <a class=\"nav-link\" href=\"/cgi-test.html\">CGI Test</a>\n"
		   << "                    </li>\n"
		   << "                    <li class=\"nav-item\">\n"
		   << "                        <a class=\"nav-link\" href=\"/upload\">Upload</a>\n"
		   << "                    </li>\n"
		   << "                </ul>\n"
		   << "            </div>\n"
		   << "        </div>\n"
		   << "    </nav>\n"
		   << "    <div class=\"container\">\n"
		   << "        <div class=\"card\">\n"
		   << "            <div class=\"card-header bg-primary text-white\">\n"
		   << "                <h1 class=\"h4 mb-0\">Directory: " << displayPath << "</h1>\n"
		   << "            </div>\n"
		   << "            <div class=\"card-body p-0\">\n"
		   << "                <div class=\"table-responsive\">\n"
		   << "                    <table class=\"table mb-0\">\n"
		   << "                        <thead class=\"table-light\">\n"
		   << "                            <tr>\n"
		   << "                                <th>Name</th>\n"
		   << "                                <th>Size</th>\n"
		   << "                                <th>Last Modified</th>\n"
		   << "                                <th>Actions</th>\n"
		   << "                            </tr>\n"
		   << "                        </thead>\n"
		   << "                        <tbody>\n";

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

	// Add parent directory link
	if (urlPath != "/" && !urlPath.empty()) {
		std::string parentPath = urlPath.substr(0, urlPath.find_last_of('/'));
		if (parentPath.empty()) parentPath = "/";
		body << "<tr>\n"
			 << "    <td><a href=\"" << parentPath << "\" class=\"text-decoration-none\">..</a></td>\n"
			 << "    <td>-</td>\n"
			 << "    <td>-</td>\n"
			 << "    <td></td>\n"
			 << "</tr>\n";
	}

	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;

		struct stat st;
		std::string fullPath = path + "/" + name;
		if (stat(fullPath.c_str(), &st) == 0) {
			std::string entryUrlPath = urlPath + "/" + FileHandler::urlDecode(name);

			body << "<tr>\n"
				 << "    <td><a href=\"" << entryUrlPath
				 << (S_ISDIR(st.st_mode) ? "/" : "")
				 << "\" class=\"text-decoration-none\">" << name << "</a></td>\n"
				 << "    <td>" << formatFileSize(st) << "</td>\n"
				 << "    <td>" << formatModTime(st) << "</td>\n"
				 << "    <td>";

			if (!S_ISDIR(st.st_mode)) {
				body << "<span class=\"delete-btn\" onclick='deleteFile(\""
					 << entryUrlPath << "\")'>Delete</span>";
			}

			body << "</td>\n"
				 << "</tr>\n";
		}
	}

	body << "                        </tbody>\n"
		 << "                    </table>\n"
		 << "                </div>\n"
		 << "            </div>\n"
		 << "        </div>\n"
		 << "    </div>\n"
		 << "    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/js/bootstrap.bundle.min.js\"></script>\n"
		 << "</body>\n"
		 << "</html>";

	return body.str();
}

std::string DirectoryHandler::formatFileSize(const struct stat &st) {
	if (S_ISDIR(st.st_mode))
		return "-";
	return Utils::numToString(st.st_size) + " bytes";
}

std::string DirectoryHandler::formatModTime(const struct stat &st) {
	char timebuf[32];
	strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
			 localtime(&st.st_mtime));
	return std::string(timebuf);
}
