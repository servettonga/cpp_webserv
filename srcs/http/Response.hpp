/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:07:58 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:31:36 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "../WebServ.hpp"
#include "../config/ServerConfig.hpp"

class Response {
	private:
		// Member variables
		int _statusCode;
		std::map<std::string, std::string> _headers;
		std::string _body;
		bool _isRawOutput;
		std::string _rawOutput;
		int _fileDescriptor;
		size_t _bytesWritten;
		bool _isStreaming;
		bool _isHeadersSent;
		std::map<std::string, std::string> _cookies;

		// Helper methods
		void updateContentLength();
		std::string getStatusText() const;
		void closeFileDescriptor();

	public:
		explicit Response(int statusCode = 200, const std::string &serverName = "webserv/1.1");
		~Response();
		void setStatusCode(int code);
		void setBody(const std::string &body);
		void addHeader(const std::string &name, const std::string &value);
		static Response makeErrorResponse(int statusCode, const ServerConfig *config = NULL);

		bool isFileDescriptor() const { return _fileDescriptor >= 0; }
		void setFileDescriptor(int fd);
		std::string toString() const;
		bool writeNextChunk(int clientFd);
		std::string getHeadersString() const;
		void setCookie(const std::string& name, const std::string& value,
					   const std::string& expires = "", const std::string& path = "/");
		void clearCookie(const std::string& name);
		void setSessionId(const std::string& sessionId);
		void clearSession();
		Response makeRedirect(int code, const std::string& location);
};

#endif
