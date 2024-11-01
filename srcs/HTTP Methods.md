# HTTP Methods

---

### **Understanding HTTP Methods**

The most common HTTP methods are:

- **GET**: Retrieves data from the server.
- **POST**: Submits data to the server.
- **PUT**: Updates existing data on the server.
- **DELETE**: Deletes data from the server.
- **HEAD**: Same as GET but only retrieves headers.
- **OPTIONS**: Describes the communication options for the target resource.

For the server, we'll focus on **GET**, **POST**, and **DELETE** methods.

---

### **Steps to Implement HTTP Methods**

1. **Update the Configuration to Specify Allowed Methods**
2. **Modify the `HTTPRequest` Class to Handle Methods**
3. **Update the `RequestHandler` to Dispatch Methods**
4. **Implement `GET` Method Handling**
5. **Implement `POST` Method Handling**
6. **Implement `DELETE` Method Handling**
7. **Send Appropriate HTTP Responses**
8. **Test The Implementation**

---

### **1. Update the Configuration to Specify Allowed Methods**

Ensure that the configuration parser and structures can parse and store the allowed methods for each location.

**Configuration Structure:**

```cpp
// ConfigParser.hpp

struct LocationConfig {
    // Existing members...
    std::vector<std::string> allowed_methods;
    // ...
};
```

**Sample Configuration File:**

```
server {
    listen 8080;
    server_name example.com;

    location / {
        root /var/www/html;
        allowed_methods GET POST DELETE;
        index index.html;
        autoindex off;
    }
}
```

The configuration parser should parse the `allowed_methods` directive and populate the `LocationConfig` accordingly.

---

### **2. Modify the `HTTPRequest` Class to Handle Methods**

Ensure the `HTTPRequest` class correctly parses the HTTP method from the request line.

```cpp
// HTTPRequest.hpp

class HTTPRequest {
public:
    HTTPRequest(const std::string& rawRequest);
    ~HTTPRequest();

    const std::string& getMethod() const;
    // Other getters...

private:
    void parseRequest(const std::string& rawRequest);

    std::string method;
    // Other members...
};

// HTTPRequest.cpp

void HTTPRequest::parseRequest(const std::string& rawRequest) {
    std::istringstream requestStream(rawRequest);
    std::string line;

    // Parse request line
    if (std::getline(requestStream, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> uri >> version;
    }

    // Parse headers and body...
}
```

---

### **3. Update the `RequestHandler` to Dispatch Methods**

Modify the `handleRequest` function to dispatch requests based on the HTTP method.

```cpp
// RequestHandler.cpp

void RequestHandler::handleRequest(const std::string& requestStr, int clientFd) {
    HTTPRequest request(requestStr);

    // Find matching server and location configurations
    const ServerConfig& serverConfig = findServerConfig(request);
    const LocationConfig& location = findLocationConfig(request, serverConfig);

    // Check if the method is allowed
    if (!isMethodAllowed(request.getMethod(), location)) {
        // Send 405 Method Not Allowed
        sendErrorResponse(clientFd, 405, "Method Not Allowed");
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
        // Send 501 Not Implemented
        sendErrorResponse(clientFd, 501, "Not Implemented");
    }
}
```

---

### **4. Implement `GET` Method Handling**

The `GET` method retrieves the specified resource. This has already been implemented when serving static files.

---

### **5. Implement `POST` Method Handling**

The `POST` method submits data to the server. We covered handling file uploads with `POST`. You can extend this method to handle other types of data submissions, such as form data.

```cpp
void RequestHandler::handlePostRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& location) {
    // Check the Content-Type
    const std::string& contentType = request.getHeader("Content-Type");
    if (contentType == "application/x-www-form-urlencoded") {
        // Handle form data
        const std::string& body = request.getBody();
        // Parse form data and process it
    } else if (contentType.find("multipart/form-data") != std::string::npos) {
        // Handle file uploads (already implemented)
    } else {
        // Unsupported Media Type
        sendErrorResponse(clientFd, 415, "Unsupported Media Type");
        return;
    }

    // Send appropriate response
    Response response;
    response.setStatusCode(200);
    response.setReasonPhrase("OK");
    response.setHeader("Content-Length", "0");
    std::string responseStr = response.toString();
    send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}
```

---

### **6. Implement `DELETE` Method Handling**

The `DELETE` method deletes the specified resource.

```cpp
void RequestHandler::handleDeleteRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& location) {
    // Sanitize and decode the URI
    std::string uri = sanitizeURI(request.getURI());
    uri = urlDecode(uri);

    // Construct file path
    std::string filePath = constructFilePath(uri, location);

    // Check if the file exists
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == -1 || !S_ISREG(fileStat.st_mode)) {
        // File not found
        sendErrorResponse(clientFd, 404, "Not Found");
        return;
    }

    // Attempt to delete the file
    if (remove(filePath.c_str()) == 0) {
        // Deletion successful
        Response response;
        response.setStatusCode(200);
        response.setReasonPhrase("OK");
        response.setHeader("Content-Length", "0");
        std::string responseStr = response.toString();
        send(clientFd, responseStr.c_str(), responseStr.size(), 0);
    } else {
        // Deletion failed
        sendErrorResponse(clientFd, 403, "Forbidden");
    }
}
```

---

### **7. Send Appropriate HTTP Responses**

Implement a method to send error responses:

```cpp
void RequestHandler::sendErrorResponse(int clientFd, int statusCode, const std::string& reasonPhrase) {
    Response response;
    response.setStatusCode(statusCode);
    response.setReasonPhrase(reasonPhrase);

    // Optionally set an error page
    std::string errorPage = getErrorPageContent(statusCode);
    if (!errorPage.empty()) {
        response.setHeader("Content-Type", "text/html");
        response.setHeader("Content-Length", std::to_string(errorPage.size()));
        response.setBody(errorPage);
    } else {
        response.setHeader("Content-Length", "0");
    }

    std::string responseStr = response.toString();
    send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}
```

---

### **8. Test The Implementation**

- **GET Requests:**
    - Access existing files and ensure they are served correctly.
    - Request non-existent files to test 404 responses.

- **POST Requests:**
    - Submit form data and validate that the server processes it correctly.
    - Upload files if applicable.

- **DELETE Requests:**
    - Delete existing files and confirm they are removed.
    - Attempt to delete non-existent files to test error handling.

- **Invalid Methods:**
    - Send requests with methods like `PUT` or `PATCH`, and ensure the server responds with `501 Not Implemented`.

- **Method Not Allowed:**
    - Send requests with methods not allowed in the location configuration, and ensure the server responds with `405 Method Not Allowed`.

---

### **Additional Considerations**

- **HEAD Method Support:**
    - Similar to `GET` but without the response body.
    - You can implement `handleHeadRequest` following the same logic as `handleGetRequest` but omitting the body in the response.

- **OPTIONS Method Support:**
    - Respond with the allowed methods for the target resource.
    - Set the `Allow` header with the list of allowed methods.

  ```cpp
  void RequestHandler::handleOptionsRequest(const HTTPRequest& request, int clientFd, const LocationConfig& location) {
      Response response;
      response.setStatusCode(204);
      response.setReasonPhrase("No Content");

      // Build the Allow header
      std::string allowMethods;
      for (size_t i = 0; i < location.allowed_methods.size(); ++i) {
          if (i > 0) allowMethods += ", ";
          allowMethods += location.allowed_methods[i];
      }
      response.setHeader("Allow", allowMethods);
      response.setHeader("Content-Length", "0");

      std::string responseStr = response.toString();
      send(clientFd, responseStr.c_str(), responseStr.size(), 0);
  }
  ```

- **Connection Management:**
    - Handle persistent connections if required by managing the `Connection` header.

- **Error Handling:**
    - Ensure all error conditions are properly handled, and informative responses are sent to the client.

- **Security:**
    - Validate input data to prevent security vulnerabilities like injection attacks.
    - Restrict file system access to prevent unauthorized access or deletion.

---
