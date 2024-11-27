/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/26 19:35:34 by jdepka           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <vector>
#include <ctime>
#include <sstream>
#include <fstream>
#include <stdexcept>

class Response {
	public:
		// Constructors and Destructor
		Response();
		explicit Response(int statusCode);
		~Response();

		// Status code management
		void setStatusCode(int code);
		int getStatusCode() const;

		// Headers management
		void addHeader(const std::string &name, const std::string &value);
		void removeHeader(const std::string &name);
		bool hasHeader(const std::string &name) const;
		std::string getHeader(const std::string &name) const;

		// Body management
		void setBody(const std::string &content);
		void appendBody(const std::string &content);
		const std::string &getBody() const;

		// Response generation
		std::string toString() const;
		std::vector<unsigned char> toBinary() const;

		// Utility methods
		void setCookie(const std::string &name,
					   const std::string &value,
					   const std::map<std::string, std::string> &options);
		void redirect(const std::string &location, int code = 302);
		void sendFile(const std::string &filePath);

	private:
		// Response components
		int _statusCode;
		std::map<std::string, std::string> _headers;
		std::string _body;

		// Helper methods
		std::string getStatusText() const;
		void setDefaultHeaders();
		void updateContentLength();
		bool isValidStatusCode(int code) const;
		std::string formatHeader(const std::string &name,
								 const std::string &value) const;
};

#endif
