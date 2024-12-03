/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 22:50:59 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/24 10:10:40 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sstream>
#include "Utils.hpp"

/*
	StringUtils Implementation
*/

/*
	split(str, delim):
	1. Initialize result vector
	2. Find first delimiter
	3. WHILE delimiter found:
	   - Extract substring
	   - Add to result
	   - Find next delimiter
	4. Add remaining string
	5. Return vector
*/

/*
	trim(str):
	1. Find first non-whitespace
	2. Find last non-whitespace
	3. Extract substring
	4. Return trimmed string
*/

std::string Utils::StringUtils::numToString(int value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

std::string Utils::StringUtils::numToString(long value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

std::string Utils::StringUtils::numToString(unsigned long value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

std::string Utils::StringUtils::numToString(long long value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

/*
	FileUtils Implementation
*/

/*
	fileExists(path):
	1. Try to access file
	2. Check error conditions
	3. Return result
*/

/*
	listDirectory(path):
	1. Open directory
	2. Read directory entries
	3. Filter special entries (. and ..)
	4. Sort entries
	5. Return list
*/

/*
	TimeUtils Implementation
*/

std::string Utils::TimeUtils::getCurrentTime() {
	/*
		getCurrentTime():
		1. Get current time
		2. Format according to ISO 8601
		3. Return formatted string
	*/
	time_t now = time(0);
	struct tm tstruct = {};
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
	return buf;
}

/*
	getHTTPDate():
	1. Get current time
	2. Format according to RFC 7231
	3. Return formatted string
*/

/*
	PathUtils Implementation
*/

/*
	normalizePath(path):
	1. Remove duplicate slashes
	2. Resolve .. and . segments
	3. Handle empty paths
	4. Return normalized path
*/

/*
	joinPath(base, path):
	1. Normalize both paths
	2. Handle absolute paths
	3. Combine paths
	4. Return joined path
*/

/*
	HTTPUtils Implementation
*/

/*
	urlEncode(str):
	1. Initialize result
	2. FOR each character:
	   IF needs encoding:
		 Add percent-encoded value
	   ELSE:
		 Add character as-is
	3. Return encoded string
*/

/*
	parseQueryString(query):
	1. Split on &
	2. FOR each pair:
	   - Split on =
	   - Decode key and value
	   - Add to map
	3. Return parsed parameters
*/

/*
	@see: https://developer.mozilla.org/en-US/docs/Web/HTTP/MIME_types/Common_types
*/
std::string Utils::HTTPUtils::getMimeType(const std::string &ext) {
	if (ext == "txt") return "text/plain";
	if (ext == "html" || ext == "htm") return "text/html";
	if (ext == "json") return "application/json";
	if (ext == "js") return "application/javascript";
	if (ext == "css") return "text/css";
	if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
	if (ext == "png") return "image/png";
	if (ext == "gif") return "image/gif";
	if (ext == "svg") return "image/svg+xml";
	if (ext == "ico") return "image/x-icon";
	if (ext == "pdf") return "application/pdf";
	if (ext == "zip") return "application/zip";
	if (ext == "tar") return "application/x-tar";
	if (ext == "xml") return "application/xml";
	if (ext == "sh") return "application/x-sh";
	return "application/octet-stream";
}

/*
	SystemUtils Implementation
*/

/*
	setNonBlocking(fd):
	1. Get current flags
	2. Add non-blocking flag
	3. Set new flags
	4. Return success/failure
*/

/*
	getErrorString(errnum):
	1. Get system error message
	2. Handle special cases
	3. Return error string
*/

