/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 19:45:02 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/06 18:30:05 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>

// Structure to hold location block configuration
struct LocationConfig {
	std::string path;                    // URL path this location handles
	std::vector<std::string> methods;    // Allowed HTTP methods (GET, POST, etc.)
	std::string root;                    // Root directory for this location
	std::string index;                   // Default index file
	bool autoindex;                      // Directory listing enabled/disabled
	std::vector<std::string> cgi_ext;    // CGI file extensions (.php, .py, etc)
	std::string cgi_path;                // Path to CGI executable
	unsigned long client_max_body_size;   // Maximum allowed request body size

	// Constructor with default values
	LocationConfig() : autoindex(false), client_max_body_size(1024 * 1024) {} // 1MB default
};

// Main server configuration structure
struct ServerConfig {
	// Network settings
	std::string host;                    // Host address to listen on
	int port;                            // Port number
	std::vector<std::string> server_names; // Server names for virtual hosting

	// Default server settings
	std::string root;                    // Server root directory
	std::string index;                   // Default index file
	unsigned int client_timeout;         // Client timeout in seconds
	unsigned long client_max_body_size;  // Maximum request body size

	// Error pages
	std::map<int, std::string> error_pages; // Custom error pages mapping

	// Locations configuration
	std::vector<LocationConfig> locations; // Location blocks

	// CGI configuration
	std::map<std::string, std::string> cgi_handlers; // Extension to handler mapping

	// Constructor with default values
	ServerConfig() :
			host("0.0.0.0"),
			port(80),
			client_timeout(60),
			client_max_body_size(1024 * 1024) // 1MB default
	{
		// Add default index file
		index = "index.html";

		// Setup default error pages
		error_pages[404] = "404.html";
		error_pages[500] = "500.html";
	}

	// Helper method to find location config for a given path
	const LocationConfig *getLocation(const std::string &path) const {
		// Implementation will search for most specific matching location
		// Return NULL if no matching location is found
		(void) path;
		return NULL;
	}

	// Helper method to check if a file extension is handled by CGI
	bool isCGIExtension(const std::string& extension) const {
		return cgi_handlers.find(extension) != cgi_handlers.end();
	}
};

#endif // SERVERCONFIG_HPP