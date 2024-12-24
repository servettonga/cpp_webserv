/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServ.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/20 19:07:05 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/25 23:45:56 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

// Common Headers

// C System
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <sys/time.h>

// C Network
# include <sys/socket.h>
# include <sys/select.h>
# include <arpa/inet.h>
# include <netinet/in.h>

// Containers
# include <map>
# include <set>
# include <vector>
# include <algorithm>
# include <iterator>
# include <list>
# include <utility>

// CPP Includes
# include <iostream>
# include <iomanip>
# include <sstream>
# include <fstream>
# include <string>
# include <limits>
# include <cstdio>
# include <cstring>

// C Includes
# include <unistd.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <fcntl.h>
# include <time.h>
# include <limits.h>
# include <errno.h>
# include <dirent.h>

// Default values
# define DEFAULT_PORT 8080
# define DEFAULT_HOST "0.0.0.0"
# define DEFAULT_CONFIG_FILE "config/default.conf"
# define DEFAULT_INDEX "index.html"
# define CGI_BUFSIZE 8192 // 8KB
# define CGI_TIMEOUT 30 // 30s
# define CGI_PIPE_BUFSIZE 1048576 // 1MB
# define RECV_SIZE 4096 // 4KB
# define RESPONSE_SIZE 8192 // 8KB
# define CHUNK_BUFFER_SIZE 655360 // 640KB
# define CLIENT_MAX_BODY 1024 * 1024 // 1MB
# define CLIENT_TIMEOUT 60 // 60s
# define KEEP_ALIVE_TIMEOUT 60 // 60s
# define SERVER_LOG "logs/server.log"

// Colors
# define WHITE "\033[37m"
# define RED "\033[31m"
# define GREEN "\033[32m"
# define YELLOW "\033[33m"
# define RESET "\033[0m"

// Utils
# include "utils/Logger.hpp"
# include "utils/Utils.hpp"

#endif