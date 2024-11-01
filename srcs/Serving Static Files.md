# Serving Static Files

---

### **1. Understanding the Requirements**

To serve static files, the server needs to:

- Parse incoming HTTP requests to determine the requested resource.
- Construct the file path by combining the server's document root with the request URI.
- Check if the requested file exists and is accessible.
- Read the file content.
- Build an HTTP response with appropriate headers (e.g., `Content-Type`, `Content-Length`).
- Send the response back to the client.

---

### **2. Updating the `RequestHandler` Class**

We will enhance the `RequestHandler` class to handle static file requests.

**`RequestHandler.hpp`:**

```cpp
#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "HTTPRequest.hpp"
#include "Response.hpp"
#include "ConfigParser.hpp"
#include <string>

class RequestHandler {
public:
    RequestHandler(const std::vector<ServerConfig>& configs);
    ~RequestHandler();

    void handleRequest(const std::string& requestStr, int clientFd);

private:
    const std::vector<ServerConfig>& serverConfigs;

    void handleGetRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& location);
    // Add methods for POST, DELETE, etc.

    // Helper methods
    std::string constructFilePath(const std::string& uri, const LocationConfig& location);
    std::string getContentType(const std::string& filePath);
    bool isMethodAllowed(const std::string& method, const LocationConfig& location);
    const ServerConfig& findServerConfig(const HTTPRequest& request);
    const LocationConfig& findLocationConfig(const HTTPRequest& request, const ServerConfig& serverConfig);

    std::string generateDirectoryListing(const std::string& directoryPath, const std::string& uriPath);
    std::string sanitizeURI(const std::string& uri);
    std::string urlDecode(const std::string& str);
};

#endif // REQUESTHANDLER_HPP
```

---

### **3. Implementing the `handleRequest` Method**

In the `handleRequest` method, we'll:

- Parse the HTTP request.
- Find the server and location configurations.
- Check if the request method is allowed.
- Call the appropriate method handler (e.g., `handleGetRequest` for GET requests).

**`RequestHandler.cpp`:**

```cpp
#include "RequestHandler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>

RequestHandler::RequestHandler(const std::vector<ServerConfig>& configs) : serverConfigs(configs) {
}

RequestHandler::~RequestHandler() {
}

void RequestHandler::handleRequest(const std::string& requestStr, int clientFd) {
    HTTPRequest request(requestStr);

    // Find matching server and location configurations
    const ServerConfig& serverConfig = findServerConfig(request);
    const LocationConfig& location = findLocationConfig(request, serverConfig);

    if (!isMethodAllowed(request.getMethod(), location)) {
        // Send 405 Method Not Allowed
        Response response;
        response.setStatusCode(405);
        response.setReasonPhrase("Method Not Allowed");
        response.setHeader("Content-Length", "0");
        std::string responseStr = response.toString();
        send(clientFd, responseStr.c_str(), responseStr.size(), 0);
        return;
    }

    if (request.getMethod() == "GET") {
        handleGetRequest(request, clientFd, serverConfig, location);
    } else {
        // Handle other methods (POST, DELETE) as needed
    }
}
```

---

### **4. Implementing the `handleGetRequest` Method**

This method processes GET requests and serves the requested static files.

```cpp
void RequestHandler::handleGetRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& location) {
    // Sanitize and decode the URI
    std::string uri = sanitizeURI(request.getURI());
    uri = urlDecode(uri);

    // Construct the file path
    std::string filePath = constructFilePath(uri, location);

    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == -1) {
        // File not found, send 404 Not Found
        Response response;
        response.setStatusCode(404);
        response.setReasonPhrase("Not Found");
        response.setHeader("Content-Length", "0");
        std::string responseStr = response.toString();
        send(clientFd, responseStr.c_str(), responseStr.size(), 0);
        return;
    }

    if (S_ISDIR(fileStat.st_mode)) {
        // Handle directories
        if (location.autoindex) {
            // Generate directory listing
            std::string directoryListing = generateDirectoryListing(filePath, uri);
            Response response;
            response.setStatusCode(200);
            response.setReasonPhrase("OK");
            response.setHeader("Content-Type", "text/html");
            response.setHeader("Content-Length", std::to_string(directoryListing.size()));
            response.setBody(directoryListing);
            std::string responseStr = response.toString();
            send(clientFd, responseStr.c_str(), responseStr.size(), 0);
            return;
        } else {
            // Try to serve the default index file
            if (!location.index.empty()) {
                filePath += "/" + location.index;
                if (stat(filePath.c_str(), &fileStat) == -1 || S_ISDIR(fileStat.st_mode)) {
                    // Index file not found
                    Response response;
                    response.setStatusCode(403);
                    response.setReasonPhrase("Forbidden");
                    response.setHeader("Content-Length", "0");
                    std::string responseStr = response.toString();
                    send(clientFd, responseStr.c_str(), responseStr.size(), 0);
                    return;
                }
            } else {
                // Index not configured
                Response response;
                response.setStatusCode(403);
                response.setReasonPhrase("Forbidden");
                response.setHeader("Content-Length", "0");
                std::string responseStr = response.toString();
                send(clientFd, responseStr.c_str(), responseStr.size(), 0);
                return;
            }
        }
    }

    // Read the file content
    std::ifstream fileStream(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!fileStream.is_open()) {
        // Error opening file
        Response response;
        response.setStatusCode(500);
        response.setReasonPhrase("Internal Server Error");
        response.setHeader("Content-Length", "0");
        std::string responseStr = response.toString();
        send(clientFd, responseStr.c_str(), responseStr.size(), 0);
        return;
    }

    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    std::string fileContent = buffer.str();
    fileStream.close();

    // Determine Content-Type
    std::string contentType = getContentType(filePath);

    // Build and send the response
    Response response;
    response.setStatusCode(200);
    response.setReasonPhrase("OK");
    response.setHeader("Content-Type", contentType);
    response.setHeader("Content-Length", std::to_string(fileContent.size()));
    response.setBody(fileContent);
    std::string responseStr = response.toString();
    send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}
```

---

### **5. Implementing Helper Methods**

**Constructing the File Path:**

```cpp
std::string RequestHandler::constructFilePath(const std::string& uri, const LocationConfig& location) {
    std::string root = location.root.empty() ? "./" : location.root;
    if (root[root.size() - 1] != '/')
        root += '/';

    std::string path = uri;
    if (!path.empty() && path[0] == '/')
        path = path.substr(1);

    return root + path;
}
```

**Determining Content-Type:**

```cpp
std::string RequestHandler::getContentType(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = filePath.substr(dotPos + 1);
        if (extension == "html" || extension == "htm") return "text/html";
        if (extension == "css") return "text/css";
        if (extension == "js") return "application/javascript";
        if (extension == "png") return "image/png";
        if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
        if (extension == "gif") return "image/gif";
        if (extension == "txt") return "text/plain";
        // Add more as needed
    }
    return "application/octet-stream";
}
```

**Checking Allowed Methods:**

```cpp
bool RequestHandler::isMethodAllowed(const std::string& method, const LocationConfig& location) {
    return std::find(location.allowed_methods.begin(), location.allowed_methods.end(), method) != location.allowed_methods.end();
}
```

---

### **6. Finding Server and Location Configurations**

**Finding the Appropriate Server Config:**

```cpp
const ServerConfig& RequestHandler::findServerConfig(const HTTPRequest& request) {
    // Simplified: return the first server config
    return serverConfigs[0];
}
```

**Finding the Matching Location Config:**

```cpp
const LocationConfig& RequestHandler::findLocationConfig(const HTTPRequest& request, const ServerConfig& serverConfig) {
    const std::string& uri = request.getURI();
    size_t longestMatch = 0;
    const LocationConfig* bestMatch = nullptr;

    for (size_t i = 0; i < serverConfig.locations.size(); ++i) {
        const std::string& path = serverConfig.locations[i].path;
        if (uri.compare(0, path.length(), path) == 0 && path.length() > longestMatch) {
            longestMatch = path.length();
            bestMatch = &serverConfig.locations[i];
        }
    }

    if (bestMatch)
        return *bestMatch;
    else
        return serverConfig.defaultLocation; // Assume a default location is defined
}
```

---

### **7. Generating Directory Listings**

```cpp
std::string RequestHandler::generateDirectoryListing(const std::string& directoryPath, const std::string& uriPath) {
    DIR* dir = opendir(directoryPath.c_str());
    if (!dir) {
        return "<html><body><h1>Unable to open directory</h1></body></html>";
    }

    std::stringstream html;
    html << "<html><head><title>Index of " << uriPath << "</title></head>";
    html << "<body><h1>Index of " << uriPath << "</h1><hr><pre>";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        html << "<a href=\"" << uriPath;
        if (uriPath[uriPath.size() - 1] != '/')
            html << '/';
        html << name << "\">" << name << "</a>\n";
    }

    closedir(dir);
    html << "</pre><hr></body></html>";
    return html.str();
}
```

---

### **8. Sanitizing the Request URI**

```cpp
std::string RequestHandler::sanitizeURI(const std::string& uri) {
    std::string sanitized;
    for (size_t i = 0; i < uri.length(); ++i) {
        if (uri[i] == '.' && i + 1 < uri.length() && uri[i + 1] == '.') {
            // Skip '..' to prevent directory traversal attacks
            i += 1;
        } else if (uri[i] == '/') {
            sanitized += '/';
            while (i + 1 < uri.length() && uri[i + 1] == '/')
                i += 1; // Skip duplicate '/'
        } else {
            sanitized += uri[i];
        }
    }
    return sanitized;
}
```

**URL Decoding:**

```cpp
#include <cstdlib>

std::string RequestHandler::urlDecode(const std::string& str) {
    std::string decoded;
    char hex[3];
    hex[2] = '\0';

    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            hex[0] = str[i + 1];
            hex[1] = str[i + 2];
            decoded += static_cast<char>(strtol(hex, NULL, 16));
            i += 2;
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}
```

---

### **9. Enhancing the `Response` Class**

Ensure that the `Response` class handles status codes and reason phrases correctly.

```cpp
// In Response.hpp
void setReasonPhrase(const std::string& phrase);

// In Response.cpp
void Response::setReasonPhrase(const std::string& phrase) {
    reasonPhrase = phrase;
}

std::string Response::toString() const {
    std::stringstream responseStream;
    responseStream << "HTTP/1.1 " << statusCode << " " << reasonPhrase << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        responseStream << it->first << ": " << it->second << "\r\n";
    }
    responseStream << "\r\n" << body;
    return responseStream.str();
}
```

---

### **10. Testing The Server**

- **Set Up The Document Root:**
    - Place some HTML, CSS, and image files in the document root directory.
    - Ensure that the `root` directive in the configuration points to this directory.

- **Configure Autoindex and Index Files:**
    - Enable `autoindex` in the configuration for certain locations if desired.
    - Specify an `index` file (e.g., `index.html`) for directories.

- **Run the Server and Test:**
    - Build and run the server.
    - Use a web browser or tools like `curl` to request different files and directories.
    - Verify that files are served correctly and that directory listings appear when expected.
    - Check that appropriate Content-Type headers are being sent.

---

### **11. Handling Errors and Edge Cases**

- **File Not Found (404):**
    - Ensure that a `404 Not Found` response is sent when a requested file doesn't exist.

- **Forbidden Access (403):**
    - Send a `403 Forbidden` response when access to a resource is denied.

- **Method Not Allowed (405):**
    - Return `405 Method Not Allowed` if the requested method is not permitted for the resource.

- **Internal Server Error (500):**
    - Use `500 Internal Server Error` for unexpected errors, such as file read failures.

- **Security Measures:**
    - Prevent directory traversal attacks by sanitizing the URI.
    - Ensure that the server doesn't serve files outside the document root.

---
