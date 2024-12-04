/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerGroup.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/04 11:30:42 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/04 11:30:58 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_GROUP_HPP
#define SERVER_GROUP_HPP

#include "Server.hpp"
#include <vector>

class ServerGroup {
	private:
		static const int SELECT_TIMEOUT_SEC = 1;
		static const int SELECT_TIMEOUT_USEC = 0;

		static ServerGroup* instance;
		static bool _shutdownRequested;
		std::vector<Server*> _servers;
		bool _isRunning;
		fd_set _masterSet;
		fd_set _readSet;
		fd_set _writeSet;
		int _maxFd;

		void updateMaxFd();
		void handleEvents(fd_set &readSet, fd_set &writeSet);

		void initializeServers();
		bool handleSelect();

		static void signalHandler(int signum);
		void cleanup();

	public:
		ServerGroup();
		~ServerGroup();

		void addServer(const ServerConfig &config);
		void start();
		void stop();

		void setupSignalHandlers();
};

#endif