/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/04 22:17:51 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include "../http/HTTPRequest.hpp"
#include "../http/Response.hpp"
#include "../server/ServerConfig.hpp"

class CGIHandler {
	public:
		// Constructor and Destructor
		CGIHandler();
		~CGIHandler();

		// Main execution method
		Response executeCGI(const HTTPRequest &request, const std::string &scriptPath);

	private:
		// Environment variables
		std::map<std::string, std::string> _envMap;
		std::string _workingDir;
		std::string _scriptPath;
		std::string _pathInfo;

		// Helper methods
		void setupEnvironment(const HTTPRequest &request);
		char** createEnvArray();
		void freeEnvArray(char** env);
		std::string unchunkData(const std::string &chunkedData);
		std::string readFromPipe(int fd);
		void writeToPipe(int fd, const std::string &data);
		void parseOutput(const std::string &output, Response &response);
};

#endif
