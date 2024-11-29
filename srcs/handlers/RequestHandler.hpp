/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 20:12:06 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "../http/HTTPRequest.hpp"
#include "../http/Response.hpp"
#include "../server/ServerConfig.hpp"
#include "DirectoryHandler.hpp"
#include "FileHandler.hpp"
#include "MimeTypes.hpp"

class RequestHandler {
	public:
		explicit RequestHandler(const ServerConfig &config);
		Response handleRequest(const HTTPRequest &request);

	private:
		Response handleGET(const HTTPRequest &request) const;
		Response handlePOST(const HTTPRequest &request) const;
		Response handleDELETE(const HTTPRequest &request) const;

		const LocationConfig *getLocation(const std::string &uri) const;
		bool isMethodAllowed(const std::string &method, const LocationConfig &loc) const;
		Response generateErrorResponse(int statusCode, const std::string &message) const;

		const ServerConfig &_config;
};

#endif
