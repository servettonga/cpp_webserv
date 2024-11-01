/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 23:07:40 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 19:35:17 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>

class HTTPRequest {
	public:
		explicit HTTPRequest(const std::string& rawRequest);
		~HTTPRequest();

		// Getters for request components
		const std::string& getMethod() const;
		const std::string& getURI() const;
		const std::string& getVersion() const;
		const std::map<std::string, std::string>& getHeaders() const;
		const std::string& getBody() const;
		const std::string& getBoundary() const;

		// Parsing functions
		void parseRequest(const std::string& rawRequest);
		void parseHeaders(const std::string& headerSection);
		void parseBody(const std::string& bodySection);

		// Form data structure
		struct FormDataPart {
			std::map<std::string, std::string> headers;
			std::string content;
		};

		// Multipart form data parsing
		std::vector<FormDataPart> parseMultipartFormData();

	private:
		// Request components
		std::string _method;
		std::string _uri;
		std::string _version;
		std::map<std::string, std::string> _headers;
		std::string _body;

		// Boundary string for multipart/form-data
		std::string _boundary;

		// Helper functions
		void parseRequestLine(const std::string& requestLine);
		void parseFormDataPart(const std::string& partData, FormDataPart& part);
};

#endif // HTTPREQUEST_HPP