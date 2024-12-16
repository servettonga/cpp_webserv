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
#include "../utils/Logger.hpp"
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <cstdlib>
#include <cerrno>

class CGIHandler {
	private:
		static Logger &_logger;
		std::map<std::string, std::string> _envMap;
		std::string _cwd;
		std::string _tmpPath;

		struct TempFiles {
			mutable FILE* inFile;
			mutable FILE* outFile;
			mutable int inFd;
			mutable int outFd;
			mutable bool cleaned;

			TempFiles() : inFile(NULL), outFile(NULL), inFd(-1), outFd(-1), cleaned(false) {}
		};

		void setupEnvironment(const HTTPRequest& request, const std::string& scriptPath);
		void cleanupTempFiles(const TempFiles &files);
		char** createEnvArray();
		bool handleTimeout(pid_t pid);
		Response handleCGIOutput(const TempFiles& files);
		Response createErrorResponse(int code, const std::string& message);

	public:
		CGIHandler();
		~CGIHandler();
		Response executeCGI(const HTTPRequest& request,
							const std::string& cgiPath,
							const std::string& scriptPath);

};

#endif
