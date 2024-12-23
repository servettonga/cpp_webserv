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
#include <csignal>
#include "../utils/Utils.hpp"

class Response {
	private:
		// Member variables
		int _statusCode;
		std::map<std::string, std::string> _headers;
		std::string _body;
		bool _isChunked;
		bool _isRawOutput;
		std::string _rawOutput;
		int _fileDescriptor;
		size_t _bytesWritten;
		bool _isStreaming;
		bool _isHeadersSent;

		// Helper methods
		void updateContentLength();
		std::string getStatusText() const;

	public:
		explicit Response(int statusCode = 200, const std::string &serverName = "webserv/1.1") :
				_statusCode(statusCode),
				_isChunked(false),
				_isRawOutput(false),
				_fileDescriptor(-1),
				_bytesWritten(0),
				_isStreaming(false),
				_isHeadersSent(false) {
			_headers["Server"] = serverName;
			if (statusCode == 100) {
				_rawOutput = "HTTP/1.1 100 Continue\r\n\r\n";
				_isRawOutput = true;
			}
		}
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
		void setFileDescriptor(int fd);
		bool writeNextChunk(int clientFd);
		bool isFileDescriptor() const { return _fileDescriptor >= 0; }
		void closeFileDescriptor();
		int getFileDescriptor() const { return _fileDescriptor; }
		std::string getHeadersString() const {
			std::string headers = "HTTP/1.1 " + Utils::numToString(_statusCode) + " " + getStatusText() + "\r\n";
			for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
				 it != _headers.end(); ++it) {
				headers += it->first + ": " + it->second + "\r\n";
			}
			headers += "\r\n";
			return headers;
		}
};

#endif
