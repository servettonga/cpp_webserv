/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/27 21:46:17 by jdepka           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <sstream>

class HTTPRequest {
	public:
		// Constructors and Destructor
		HTTPRequest();
		explicit HTTPRequest(const std::string &rawRequest);
		~HTTPRequest();

		// Form data structure for multipart/form-data
		struct FormDataPart {
			std::map<std::string, std::string> headers;
			std::string content;
		};

		// Getters
		const std::string &getMethod() const;
		const std::string &getURI() const;
		const std::string &getVersion() const;
		const std::map<std::string, std::string> &getHeaders() const;
		const std::string &getBody() const;
		const std::string &getBoundary() const;

		// Parsing methods
		void parseRequest(const std::string &rawRequest);
		void parseHeaders(const std::string &headerSection);
		void parseBody(const std::string &bodySection);
		std::vector<FormDataPart> parseMultipartFormData();

	private:
		// Request components
		std::string _method;
		std::string _uri;
		std::string _version;
		std::map<std::string, std::string> _headers;
		std::string _body;
		std::string _boundary;

		// Helper methods
		void parseRequestLine(const std::string &requestLine);
		void parseFormDataPart(const std::string &partData, FormDataPart &part);
		static bool isValidMethod(const std::string &method);
		void extractQueryString();
		void normalizePath();
		static std::string decodeURIComponent(const std::string &encoded);
};

#endif
