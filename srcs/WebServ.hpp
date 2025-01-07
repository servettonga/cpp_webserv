/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServ.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/20 19:07:05 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 23:00:18 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

// Common Headers

// C System
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

// C Network
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

// Containers
#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>

// CPP Includes
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

// C Includes
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Default values
#define DEFAULT_PORT 8080
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_CONFIG_FILE "config/default.conf"
#define DEFAULT_INDEX "index.html"
#define CGI_BUFSIZE 8192			// 8KB
#define CGI_TIMEOUT 30				// 30s
#define CGI_PIPE_BUFSIZE 1048576	// 1MB
#define RECV_SIZE 4096				// 4KB
#define RESPONSE_SIZE 8192			// 8KB
#define CHUNK_BUFFER_SIZE 655360	// 640KB
#define CLIENT_MAX_BODY 1024 * 1024 // 1MB
#define CLIENT_TIMEOUT 60			// 60s
#define KEEP_ALIVE_TIMEOUT 60		// 60s
#define SERVER_LOG "logs/server.log"

// Utils
#include "utils/Utils.hpp"

#endif
