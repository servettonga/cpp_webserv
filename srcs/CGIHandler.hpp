/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/02 20:01:02 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include "HTTPRequest.hpp"
#include "Response.hpp"
#include "ConfigParser.hpp"

class CGIHandler {
	private:
		std::map<std::string, std::string> _environMap;
		std::string _workingDir;
		std::string _scriptPath;
		std::string _pathInfo;

		// Helper methods
		void setupEnvironment(const HTTPRequest& request, const std::string& scriptPath);
		char** createEnvArray() const;
		void freeEnvArray(char** env) const;
		static std::string unchunkData(const std::string& chunkedData);
		static std::string readFromPipe(int fd);
		static void writeToPipe(int fd, const std::string& data);
		static std::string getCGIExecutable(const std::string& extension) ;
		static void parseScriptOutput(const std::string& output, Response& response);

	public:
		CGIHandler();
		~CGIHandler();

		Response executeCGI(const HTTPRequest& request,
							const std::string& scriptPath,
							const std::string& cgiRoot);
};

#endif // CGIHANDLER_HPP