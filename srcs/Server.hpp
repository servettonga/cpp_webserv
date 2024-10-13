/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/26 12:27:08 by sehosaf           #+#    #+#             */
/*   Updated: 2024/10/28 15:46:11 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

class Server {
	public:
		explicit Server(const std::map<std::string, std::string> &config);
		bool initialize();
		void run();

	private:
		std::map<std::string, std::string> config;
		int                                listen_fd;
		std::vector<pollfd>                poll_fds;

		bool setupSocket();
		void handleConnections();
		static void handleRequest(int client_fd);
};

#endif // SERVER_HPP
