/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/24 12:49:07 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
	private:
		int _statusCode;
		std::map<std::string, std::string> _headers;
		std::string _body;

		void updateContentLength();
		std::string getStatusText() const;

	public:
		Response(int statusCode = 200, const std::string &serverName = "webserv/1.1");
		~Response();

		void setStatusCode(int code);
		void setBody(const std::string& body);

		void addHeader(const std::string& name, const std::string& value);

		std::string toString() const;
};

#endif
