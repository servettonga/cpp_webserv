/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 19:45:02 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 19:15:43 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "../WebServ.hpp"

struct LocationConfig {
	std::string path;					// URL path this location handles
	std::vector<std::string> methods;	// Allowed HTTP methods (GET, POST, etc.)
	std::string root;					// Root directory for this location
	std::string index;					// Default index file
	bool autoindex;						// Directory listing enabled/disabled
	std::vector<std::string> cgi_ext;	// CGI file extensions (.php, .py, etc)
	std::string cgi_path;				// Path to CGI executable
	unsigned long client_max_body_size;	// Maximum request body size
	std::string redirect;				// Store redirect target

	LocationConfig()
		: autoindex(false), client_max_body_size(CLIENT_MAX_BODY), redirect("") {}
};

// Main server configuration structure
struct ServerConfig {
	// Network settings
	std::string host;					  // Host address to listen on
	int port;							  // Port number
	std::vector<std::string> server_names; // Server names for virtual hosting

	// Default server settings
	std::string root;					// Server root directory
	std::string index;					// Default index file
	unsigned int client_timeout;		// Client timeout in seconds
	unsigned long client_max_body_size;	// Maximum request body size

	// Error pages
	std::map<int, std::string> error_pages; // Custom error pages mapping

	// Locations configuration
	std::vector<LocationConfig> locations;  // Location blocks

	// CGI configuration
	std::map<std::string, std::string> cgi_handlers; // Extension to handler mapping

	// Constructor with default values
	ServerConfig() :
			host(DEFAULT_HOST),
			port(DEFAULT_PORT),
			client_timeout(CLIENT_TIMEOUT),
			client_max_body_size(CLIENT_MAX_BODY) { // 1MB default
		index = DEFAULT_INDEX;
		error_pages[404] = "/404.html";
		error_pages[500] = "/500.html";
	}

	// Copy constructor
	ServerConfig(const ServerConfig &other) :
			host(other.host),
			port(other.port),
			server_names(other.server_names),
			root(other.root),
			index(other.index),
			client_timeout(other.client_timeout),
			client_max_body_size(other.client_max_body_size),
			error_pages(other.error_pages),
			locations(other.locations),
			cgi_handlers(other.cgi_handlers) {
		cgiExtCache.clear();
	}

	// Add assignment operator
	ServerConfig &operator=(const ServerConfig &other) {
		if (this != &other) {
			host = other.host;
			port = other.port;
			server_names = other.server_names;
			root = other.root;
			index = other.index;
			client_timeout = other.client_timeout;
			client_max_body_size = other.client_max_body_size;
			error_pages = other.error_pages;
			locations = other.locations;
			cgi_handlers = other.cgi_handlers;
			cgiExtCache.clear();
		}
		return *this;
	}

	// Add cache for commonly accessed values
	mutable std::map<std::string, bool> cgiExtCache;

	// Pre-compute just CGI extensions
	void precomputePaths() {
		// Pre-compute CGI extensions
		for (std::map<std::string, std::string>::const_iterator it = cgi_handlers.begin();
			 it != cgi_handlers.end(); ++it) {
			cgiExtCache[it->first] = true;
		}
	}
};

#endif
