/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/02 17:41:57 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
	private:
		// Member variables
		int _statusCode;
		std::map<std::string, std::string> _headers;
		std::string _body;
		bool _isChunked;
		bool _isRawOutput;
		std::string _rawOutput;

		// Helper methods
		void updateContentLength();
		std::string getStatusText() const;

	public:
		explicit Response(int statusCode = 200, const std::string &serverName = "webserv/1.1");
		~Response();

		void setStatusCode(int code);
		void setBody(const std::string &body);
		std::string getBody();
		void setRawOutput(const std::string &output);
		void addHeader(const std::string &name, const std::string &value);
		static Response makeErrorResponse(int statusCode);
		std::string toString() const;
		void setChunked(bool chunked);
		bool isChunked() const;
		void clearHeaders();
		bool hasHeader(const char *string);

		std::string getHeader(const char *name);

		std::map<std::string, std::string> getHeaders();

		int getStatusCode();
};

#endif
