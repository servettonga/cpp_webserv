/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerGroup.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/04 11:30:42 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/10 13:15:51 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_GROUP_HPP
#define SERVER_GROUP_HPP

#include "../WebServ.hpp"
#include "Server.hpp"

class ServerGroup {
	private:
		static ServerGroup *_instance;
		static std::string _configFile;
		std::vector<Server *> _servers;

		fd_set _masterSet;
		fd_set _readSet;
		fd_set _writeSet;

		static const int SELECT_TIMEOUT_SEC = 1;
		static const int SELECT_TIMEOUT_USEC = 0;

		static bool _shutdownRequested;
		bool _isRunning;
		int _maxFd;

		void updateMaxFd();
		void handleEvents(fd_set &readSet, fd_set &writeSet);

		void initializeServers();
		bool handleSelect();
		void reloadConfiguration(const std::string &configFile);

		static void signalHandler(int signum);
		void cleanup();

	public:
		explicit ServerGroup(const std::string &configFile);
		~ServerGroup();

		void addServer(const ServerConfig &config);
		void start();
		void stop();

		void setupSignalHandlers();
};

#endif