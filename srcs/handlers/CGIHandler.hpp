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
#include <fcntl.h>

class CGIHandler {
	private:
		static Logger &_logger;
		std::map<std::string, std::string> _envMap;
		std::string _cwd;
		std::string _tmpPath;

		struct TempFiles {
			FILE* inputFile;
			FILE* outputFile;
			std::string inputPath;
			std::string outputPath;
			bool cleaned;

			TempFiles() : inputFile(NULL), outputFile(NULL), cleaned(false) {}
			~TempFiles() {
				cleanup();
			}

			void cleanup() {
				if (!cleaned) {
					if (inputFile) fclose(inputFile);
					if (outputFile) fclose(outputFile);
					if (!inputPath.empty()) unlink(inputPath.c_str());
					if (!outputPath.empty()) unlink(outputPath.c_str());
					cleaned = true;
				}
			}
		};

		#define MAX_CGI_OUTPUT_SIZE 1024 * 1024

		void setupEnvironment(const HTTPRequest& request, const std::string& scriptPath);
		void cleanupTempFiles(TempFiles &files);
		char** createEnvArray();
		bool handleTimeout(pid_t pid);
		Response handleCGIOutput(int output_fd, pid_t pid, int timeout_seconds);
		Response createErrorResponse(int code, const std::string& message);
		void setNonBlocking(int fd) const;
		Response parseCGIOutput(const std::string& output);
		void cleanup(char** env);

	public:
		CGIHandler();
		~CGIHandler();
		Response executeCGI(const HTTPRequest& request,
							const std::string& cgiPath,
							const std::string& scriptPath);

};

#endif
