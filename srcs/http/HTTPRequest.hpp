/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 17:32:57 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>

class HTTPRequest {
	public:
		HTTPRequest() : _config(NULL) {}

		// Core parsing
		bool parse(const std::string &rawRequest);

		// Getters and setters
		void setConfig(const void* config);
		const void* getConfig() const;
		const std::string &getMethod() const;
		const std::string &getPath() const;
		const std::string &getVersion() const;
		const std::string &getBody() const;
		const std::map<std::string, std::string> &getHeaders() const;
		void setTempFilePath(const std::string& path);
		const std::string &getTempFilePath() const;
		bool isChunked() const;

		// Header operations
		bool hasHeader(const std::string &name) const;
		std::string getHeader(const std::string &name) const;
		static std::string unchunkData(const std::string& chunkedData);

	private:
		// Request components
		std::string _method;
		std::string _path;
		std::string _version;
		std::map<std::string, std::string> _headers;
		std::string _body;
		const void *_config;
		std::string _tempFilePath;

		// Chunked transfer encoding
		bool _isChunked;

		bool parseChunkedBody(const std::string& rawRequest, size_t bodyStart);

		// Parsing helpers
		bool parseRequestLine(const std::string &line);
		bool parseHeaders(const std::string &headerSection);
		static std::string trimWhitespace(const std::string &str);
};

#endif
