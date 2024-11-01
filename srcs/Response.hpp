/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/01 22:57:04 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
	public:
		Response();
		Response(const Response& other);
		Response& operator=(const Response& other);
		~Response();

		void setStatusCode(int code);
		void setHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);
		std::string toString() const;

	private:
		int _statusCode;
		std::map<std::string, std::string> _headers;
		std::string _body;

		std::string getReasonPhrase() const;
		void updateContentLength();
};

#endif // RESPONSE_HPP
