/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/23 22:32:22 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>

class HTTPRequest {
	private:
		std::string _method;			// HTTP method (GET, POST, etc.)
		std::string _path;				// Request URL path
		std::string _version;			// HTTP version
		std::map<std::string, std::string> _headers;	// Request headers
		std::string _body;				 // Request body

		bool parseRequestLine(const std::string &line);
		bool parseHeaders(std::string &request);
		void parseBody(std::string &request);
		static std::string trimWhitespace(const std::string &str);

	public:
		bool parse(const std::string &raw);

		const std::string &getMethod() const;
		const std::string &getPath() const;
		const std::string &getVersion() const;
		const std::string &getBody() const;
		const std::map<std::string, std::string> &getHeaders() const;
		bool hasHeader(const std::string &name) const;
		std::string getHeader(const std::string &name) const;
};

#endif
