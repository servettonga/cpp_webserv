/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/25 12:47:08 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../WebServ.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"

class CGIHandler {
	private:
		static Logger &_logger;
		std::map<std::string, std::string> _envMap;
		std::string _cwd;
		std::string _tmpPath;

		Response handleCGIOutput(int output_fd, pid_t pid);
		Response parseCGIOutput(int raw_fd, size_t raw_bytes);
		void setupEnvironment(const Request& request, const std::string& scriptPath);
		char **createEnvArray();
		Response createErrorResponse(int code, const std::string& message);
		void cleanup(char** env);

	public:
		CGIHandler();
		~CGIHandler();
		Response executeCGI(const Request& request,
							const std::string& cgiPath,
							const std::string& scriptPath);
};

#endif
