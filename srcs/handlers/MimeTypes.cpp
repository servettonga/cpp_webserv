/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MimeTypes.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:11:38 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 20:12:37 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "MimeTypes.hpp"

namespace MimeTypes {
	std::string getType(const std::string &path) {
		size_t dot = path.find_last_of('.');
		if (dot == std::string::npos)
			return "application/octet-stream";

		std::string ext = path.substr(dot + 1);

		// Text files
		if (ext == "html" || ext == "htm") return "text/html";
		if (ext == "css") return "text/css";
		if (ext == "js") return "application/javascript";
		if (ext == "json") return "application/json";
		if (ext == "txt") return "text/plain";
		if (ext == "xml") return "application/xml";

		// Images
		if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
		if (ext == "png") return "image/png";
		if (ext == "gif") return "image/gif";
		if (ext == "svg") return "image/svg+xml";
		if (ext == "ico") return "image/x-icon";

		// Documents
		if (ext == "pdf") return "application/pdf";

		// Binary/Compressed
		if (ext == "zip") return "application/zip";
		if (ext == "tar") return "application/x-tar";
		if (ext == "gz") return "application/gzip";

		return "application/octet-stream";
	}
}
