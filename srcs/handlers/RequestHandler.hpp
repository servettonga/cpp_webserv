/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/31 20:05:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/25 22:41:39 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "../WebServ.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../config/ServerConfig.hpp"

class RequestHandler {
	private:
		const ServerConfig &_config;

		Response handleGET(const Request &request) const;
		Response handlePOST(const Request &request) const;
		Response handleDELETE(const Request &request) const;
		Response handlePUT(const Request &request) const;

		const LocationConfig *getLocation(const std::string &uri) const;
		bool isMethodAllowed(const std::string &method, const LocationConfig &loc) const;
		std::string findFirstExistingIndex(const std::string &dirPath, const std::string &indexFiles) const;

		void handleCookies(const Request &request, Response &response) const;

	public:
		explicit RequestHandler(const ServerConfig &config);
		Response handleRequest(const Request &request);
};

#endif
