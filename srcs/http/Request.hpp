/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/25 23:04:57 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "../WebServ.hpp"

class Request {
	public:
		Request() : _config(NULL), _cookies(), _isChunked(false) {}
		Request(const Request &other);
		Request &operator=(const Request &other);

		// Core parsing
		bool parse(const std::string &rawRequest);

		// Getters and setters
		void setConfig(const void* config);
		const std::string &getMethod() const;
		const std::string &getPath() const;
		const std::string &getBody() const;
		void loadBodyFromTempFile();
		void setTempFilePath(const std::string& path);
		bool isChunked() const;

		// Header operations
		bool hasHeader(const std::string &name) const;
		std::string getHeader(const std::string &name) const;
		bool parseHeaders(const std::string &headerSection);

		void clearBody();
		std::map<std::string, std::string> getCookies() const;

	private:
		std::string _method;
		std::string _path;
		std::string _queryString;
		std::string _version;
		std::map<std::string, std::string> _headers;
		std::string _body;
		const void *_config;
		std::string _tempFilePath;
		std::map<std::string, std::string> _cookies;
		bool _isChunked;

		// Parsing helpers
		bool parseRequestLine(const std::string &line);
		void parseCookies();
		static std::string trimWhitespace(const std::string &str);
		static std::string unchunkData(const std::string &chunkedData);
};

#endif
