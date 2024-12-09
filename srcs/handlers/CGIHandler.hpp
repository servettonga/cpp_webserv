/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/09 12:25:26 by jdepka           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include "../http/HTTPRequest.hpp"
#include "../http/Response.hpp"
#include "../server/ServerConfig.hpp"
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <cstring>
#include <sys/wait.h>
#include <cstdlib>
#include <cerrno>

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
