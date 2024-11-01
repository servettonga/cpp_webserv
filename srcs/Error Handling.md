# Error Handling

---

### **1. Understanding HTTP Status Codes**

HTTP status codes inform the client about the result of their request. They are categorized into:

- **1xx**: Informational responses
- **2xx**: Successful responses
- **3xx**: Redirection messages
- **4xx**: Client errors
- **5xx**: Server errors

Handling various status codes appropriately in the server is needed.

---

### **2. Defining Error Pages**

Create custom error pages for different HTTP error codes to provide users with helpful information.

**Example:**

```cpp
// In the configuration

server {
    listen 8080;
    server_name example.com;

    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;
    # Add other error pages as needed
}
```

Ensure the configuration parser can read the `error_page` directive and associate error codes with corresponding file paths.

---

### **3. Updating the Configuration Structures**

```cpp
// In ConfigParser.hpp

struct ServerConfig {
    // Existing members...
    std::map<int, std::string> error_pages;
    // ...
};
```

---

### **4. Enhancing the `Response` Class**

Modify the `Response` class to handle error responses.

```cpp
// In Response.hpp

class Response {
public:
    // Existing methods...
    void setStatusCode(int code);
    void setReasonPhrase(const std::string& phrase);
    void setBody(const std::string& content);

    std::string toString() const;

private:
    int statusCode;
    std::string reasonPhrase;
    std::map<std::string, std::string> headers;
    std::string body;
};
```

---

### **5. Implementing Error Handling Methods**

Create a method to generate error responses.

```cpp
// In RequestHandler.cpp

void RequestHandler::sendErrorResponse(int clientFd, int statusCode, const ServerConfig& serverConfig) {
    Response response;
    response.setStatusCode(statusCode);
    response.setReasonPhrase(getReasonPhrase(statusCode));

    // Check if a custom error page is configured
    std::map<int, std::string>::const_iterator it = serverConfig.error_pages.find(statusCode);
    if (it != serverConfig.error_pages.end()) {
        std::string errorPagePath = it->second;
        std::string errorPageContent = readFileContent(errorPagePath);
        if (!errorPageContent.empty()) {
            response.setHeader("Content-Type", "text/html");
            response.setHeader("Content-Length", std::to_string(errorPageContent.size()));
            response.setBody(errorPageContent);
        } else {
            // If error page can't be read, send default message
            std::string defaultMessage = "<html><body><h1>" + std::to_string(statusCode) + " " + response.getReasonPhrase() + "</h1></body></html>";
            response.setHeader("Content-Type", "text/html");
            response.setHeader("Content-Length", std::to_string(defaultMessage.size()));
            response.setBody(defaultMessage);
        }
    } else {
        // No custom error page, send default message
        std::string defaultMessage = "<html><body><h1>" + std::to_string(statusCode) + " " + response.getReasonPhrase() + "</h1></body></html>";
        response.setHeader("Content-Type", "text/html");
        response.setHeader("Content-Length", std::to_string(defaultMessage.size()));
        response.setBody(defaultMessage);
    }

    std::string responseStr = response.toString();
    send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}
```

---

### **6. Handling Exceptions and Errors in Request Processing**

Update the request handling methods to catch exceptions and errors, and send appropriate responses.

**Example in `handleRequest`:**

```cpp
void RequestHandler::handleRequest(const std::string& requestStr, int clientFd) {
    try {
        HTTPRequest request(requestStr);

        // Find matching server and location configurations
        const ServerConfig& serverConfig = findServerConfig(request);
        const LocationConfig& location = findLocationConfig(request, serverConfig);

        // Check if the method is allowed
        if (!isMethodAllowed(request.getMethod(), location)) {
            sendErrorResponse(clientFd, 405, serverConfig);
            return;
        }

        // Dispatch based on method
        const std::string& method = request.getMethod();
        if (method == "GET") {
            handleGetRequest(request, clientFd, serverConfig, location);
        } else if (method == "POST") {
            handlePostRequest(request, clientFd, serverConfig, location);
        } else if (method == "DELETE") {
            handleDeleteRequest(request, clientFd, serverConfig, location);
        } else {
            sendErrorResponse(clientFd, 501, serverConfig);
        }
    } catch (const std::exception& e) {
        // Log the error
        std::cerr << "Error handling request: " << e.what() << std::endl;
        // Send 500 Internal Server Error
        sendErrorResponse(clientFd, 500, serverConfig);
    }
}
```

---

### **7. Defining Reason Phrases**

Implement a method to get the standard reason phrase for a status code.

```cpp
// In Response.cpp

std::string getReasonPhrase(int statusCode) {
    switch(statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        // Add other status codes as needed
        default: return "Unknown Status";
    }
}
```

---

### **8. Reading Error Page Files**

Implement a helper function to read the content of error page files.

```cpp
// In RequestHandler.cpp

std::string RequestHandler::readFileContent(const std::string& filePath) {
    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        // Log error if necessary
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
```

---

### **9. Handling Invalid Requests**

Ensure the server can handle invalid or malformed HTTP requests gracefully.

**Example:**

```cpp
try {
    HTTPRequest request(requestStr);
    // Process request
} catch (const ParseException& e) {
    // Send 400 Bad Request
    sendErrorResponse(clientFd, 400, serverConfig);
}
```

You may need to define a custom `ParseException` class that is thrown when the request parsing fails.

---

### **10. Managing Timeouts and Resource Limits**

Implement timeouts for operations like reading requests, executing CGI scripts, or waiting for I/O, to prevent resource exhaustion.

**Example:**

- Use `setsockopt` to set socket timeouts.
- Implement a maximum time limit for CGI script execution.
- Limit the size of uploaded files according to `client_max_body_size`.

---

### **11. Logging Errors**

Implement a logging mechanism to record errors and other important events, which is crucial for debugging and monitoring.

**Example:**

```cpp
// Logger.hpp

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

class Logger {
public:
    static void log(const std::string& message);
    static std::string currentDateTime();
};

#endif // LOGGER_HPP
```

```cpp
// Logger.cpp

#include "Logger.hpp"
#include <fstream>
#include <ctime>

void Logger::log(const std::string& message) {
    std::ofstream logFile("server.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << currentDateTime() << " - " << message << std::endl;
        logFile.close();
    }
}

std::string Logger::currentDateTime() {
    std::time_t now = std::time(0);
    char buf[80];
    std::tm* timeinfo = std::localtime(&now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %X", timeinfo);
    return buf;
}
```

Use the logger throughout the server code:

```cpp
// In exception handling
catch (const std::exception& e) {
    Logger::log(std::string("Exception: ") + e.what());
    sendErrorResponse(clientFd, 500, serverConfig);
}
```

---

### **12. Implementing Signal Handling**

Handle signals like `SIGPIPE`, which may occur if the client closes the connection unexpectedly.

**Example:**

```cpp
// In main.cpp

#include <csignal>

void handleSignal(int signal) {
    if (signal == SIGPIPE) {
        // Handle SIGPIPE
    }
}

int main() {
    // Set up signal handler
    std::signal(SIGPIPE, handleSignal);

    // Start server loop
}
```

Alternatively, you can ignore `SIGPIPE`:

```cpp
std::signal(SIGPIPE, SIG_IGN);
```

---

### **13. Validating Configuration**

Ensure that the configuration parser validates the configuration file and handles errors appropriately.

**Example:**

```cpp
// In ConfigParser.cpp

if (invalidDirective) {
    throw ConfigException("Invalid directive at line " + std::to_string(line));
}

// In main.cpp

try {
    ConfigParser configParser("config.conf");
} catch (const ConfigException& e) {
    std::cerr << "Configuration Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
```

Define the `ConfigException` class:

```cpp
// In ConfigException.hpp

#ifndef CONFIGEXCEPTION_HPP
#define CONFIGEXCEPTION_HPP

#include <stdexcept>

class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message)
        : std::runtime_error(message) {}
};

#endif // CONFIGEXCEPTION_HPP
```

---

### **14. Handling Client Disconnects**

Detect when a client disconnects and clean up resources accordingly.

**Example:**

- Use non-blocking sockets and check for disconnects using `recv` returning zero.
- Remove the client from the connection list and close the socket.

```cpp
ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
if (bytesRead == 0) {
    // Client disconnected
    close(clientFd);
    // Clean up resources
}
```

---

### **15. Graceful Shutdown**

Implement a way to gracefully shut down the server, closing all connections and releasing resources.

**Example:**

```cpp
bool running = true;

void handleSignal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = false;
    }
}

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    while (running) {
        // Server loop
    }

    // Clean up and close sockets
    return 0;
}
```

---
