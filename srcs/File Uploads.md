# File Uploads in the HTTP Server

To implement **File Uploads** in the HTTP server, it needs to handle HTTP `POST` requests with `multipart/form-data` encoding.
This allows clients to upload files to the server, which you can then save to a specified directory.

---

### **Steps to Implement File Uploads**

1. **Update the Configuration**
    - Ensure the `LocationConfig` structure includes an `upload_path` where files will be saved.
    - Specify allowed methods (`POST`) in the configuration for the upload endpoint.

2. **Modify the `HTTPRequest` Class**
    - Enhance the request parsing to handle `multipart/form-data`.
    - Extract the `Content-Type` header and determine the boundary.

3. **Parse Multipart Form Data**
    - Implement a parser to split the request body into parts based on the boundary.
    - For each part, parse headers such as `Content-Disposition` to extract the filename and content.

4. **Save Uploaded Files**
    - Write the extracted file content to the server's filesystem at the specified `upload_path`.
    - Use the original filename or generate a unique one to prevent overwriting existing files.

5. **Handle Responses**
    - Send an appropriate HTTP response indicating the success or failure of the file upload.
    - Use status codes like `201 Created` for successful uploads.

---

### **1. Updating Configuration Structures**

Ensure the configuration supports file uploads:

```cpp
// In ConfigParser.hpp

struct LocationConfig {
    // Existing members...
    std::string upload_path;
    // Add other necessary configurations
};
```

Define the `upload_path` in the configuration file:

```conf
location /upload {
    allowed_methods POST;
    upload_path /var/www/uploads;
    # Other configurations...
}
```

---

### **2. Modifying the `HTTPRequest` Class**

Update `HTTPRequest.hpp` and `HTTPRequest.cpp` to parse the `Content-Type` header and handle `multipart/form-data`.

**Extracting the Boundary Parameter:**

```cpp
// In HTTPRequest.cpp

void HTTPRequest::parseRequest(const std::string& rawRequest) {
    // Existing parsing logic...

    // Extract Content-Type header
    std::map<std::string, std::string>::iterator it = headers.find("Content-Type");
    if (it != headers.end()) {
        std::string contentType = it->second;
        if (contentType.find("multipart/form-data") != std::string::npos) {
            size_t boundaryPos = contentType.find("boundary=");
            if (boundaryPos != std::string::npos) {
                boundary = "--" + contentType.substr(boundaryPos + 9);
            }
        }
    }
}
```

Add a member variable `boundary` to store the boundary string.

```cpp
// In HTTPRequest.hpp

class HTTPRequest {
public:
    // Existing members...
    const std::string& getBoundary() const;

private:
    // Existing members...
    std::string boundary;
};
```

---

### **3. Parsing Multipart Form Data**

Implement a method to parse the multipart data:

```cpp
// In HTTPRequest.cpp

#include <vector>

struct FormDataPart {
    std::map<std::string, std::string> headers;
    std::string content;
};

std::vector<FormDataPart> HTTPRequest::parseMultipartFormData() {
    std::vector<FormDataPart> parts;
    if (boundary.empty()) {
        return parts;
    }

    size_t pos = body.find(boundary);
    while (pos != std::string::npos) {
        size_t start = pos + boundary.length();
        size_t end = body.find(boundary, start);
        if (end == std::string::npos) {
            break;
        }

        std::string partData = body.substr(start, end - start);
        FormDataPart part;
        parseFormDataPart(partData, part);
        parts.push_back(part);

        pos = end;
    }
    return parts;
}

void HTTPRequest::parseFormDataPart(const std::string& partData, FormDataPart& part) {
    size_t headerEnd = partData.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return;
    }

    std::string headersStr = partData.substr(0, headerEnd);
    std::istringstream headerStream(headersStr);
    std::string line;

    // Parse headers
    while (std::getline(headerStream, line) && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string headerName = line.substr(0, colonPos);
            std::string headerValue = line.substr(colonPos + 1);
            part.headers[headerName] = headerValue;
        }
    }

    // Extract content
    part.content = partData.substr(headerEnd + 4);
}
```

---

### **4. Updating the `RequestHandler` Class**

Modify `handleRequest` to handle `POST` requests:

```cpp
void RequestHandler::handleRequest(const std::string& requestStr, int clientFd) {
    HTTPRequest request(requestStr);

    // Find matching server and location configurations
    const ServerConfig& serverConfig = findServerConfig(request);
    const LocationConfig& location = findLocationConfig(request, serverConfig);

    if (!isMethodAllowed(request.getMethod(), location)) {
        // Send 405 Method Not Allowed
        sendErrorResponse(clientFd, 405, "Method Not Allowed");
        return;
    }

    if (request.getMethod() == "POST") {
        handlePostRequest(request, clientFd, serverConfig, location);
    } else {
        // Handle other methods (GET, DELETE) as before
    }
}
```

---

### **5. Implementing the `handlePostRequest` Method**

Handle file uploads in the `handlePostRequest` method:

```cpp
void RequestHandler::handlePostRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& location) {
    // Check if the request is multipart/form-data
    if (request.getBoundary().empty()) {
        // Unsupported Media Type
        sendErrorResponse(clientFd, 415, "Unsupported Media Type");
        return;
    }

    // Parse multipart form data
    std::vector<FormDataPart> parts = request.parseMultipartFormData();

    // Process each part
    for (size_t i = 0; i < parts.size(); ++i) {
        std::map<std::string, std::string>& headers = parts[i].headers;
        std::string contentDisposition = headers["Content-Disposition"];

        // Extract filename from Content-Disposition
        size_t filenamePos = contentDisposition.find("filename=\"");
        if (filenamePos != std::string::npos) {
            size_t start = filenamePos + 10;
            size_t end = contentDisposition.find("\"", start);
            if (end != std::string::npos) {
                std::string filename = contentDisposition.substr(start, end - start);

                // Save the file
                std::string filePath = location.upload_path + "/" + filename;
                std::ofstream outFile(filePath.c_str(), std::ios::binary);
                if (outFile.is_open()) {
                    outFile.write(parts[i].content.c_str(), parts[i].content.size());
                    outFile.close();
                } else {
                    // Failed to save the file
                    sendErrorResponse(clientFd, 500, "Internal Server Error");
                    return;
                }
            }
        }
    }

    // Send success response
    Response response;
    response.setStatusCode(201);
    response.setReasonPhrase("Created");
    response.setHeader("Content-Length", "0");
    std::string responseStr = response.toString();
    send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}
```

---

### **6. Helper Methods**

**Sending Error Responses:**

```cpp
void RequestHandler::sendErrorResponse(int clientFd, int statusCode, const std::string& reasonPhrase) {
    Response response;
    response.setStatusCode(statusCode);
    response.setReasonPhrase(reasonPhrase);
    response.setHeader("Content-Length", "0");
    std::string responseStr = response.toString();
    send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}
```

---

### **7. Ensuring Security**

- **File Path Sanitization:** Ensure that the filename does not contain path traversal characters (`../`) to prevent writing outside the intended directory.
- **Unique Filenames:** To prevent overwriting existing files or conflicts, generate unique filenames using timestamps or UUIDs.
- **Permissions:** Set appropriate file permissions for the uploaded files.

**Sanitizing the Filename:**

```cpp
std::string RequestHandler::sanitizeFilename(const std::string& filename) {
    std::string sanitized;
    for (size_t i = 0; i < filename.length(); ++i) {
        if (filename[i] == '/' || filename[i] == '\\' || filename[i] == ':') {
            // Skip invalid characters
            continue;
        }
        sanitized += filename[i];
    }
    return sanitized;
}
```

Use `sanitizeFilename` before saving the file.

---

### **8. Handling Client Body Size Limits**

Enforce limits on the size of the uploaded files based on `client_max_body_size`:

```cpp
void RequestHandler::handlePostRequest(const HTTPRequest& request, int clientFd, const ServerConfig& serverConfig, const LocationConfig& location) {
    size_t maxBodySize = location.client_max_body_size;
    if (request.getBody().size() > maxBodySize) {
        // Payload Too Large
        sendErrorResponse(clientFd, 413, "Payload Too Large");
        return;
    }

    // Proceed with parsing and saving the file
}
```

---

### **9. Testing File Uploads**

- **Create an HTML Form:**

  ```html
  <form action="http://localhost:8080/upload" method="POST" enctype="multipart/form-data">
      Select file to upload:
      <input type="file" name="file">
      <input type="submit" value="Upload File">
  </form>
  ```

- **Use `curl` Command:**

  ```bash
  curl -F "file=@/path/to/the/file.txt" http://localhost:8080/upload
  ```

- **Verify Server Behavior:**
    - Ensure the file is saved in the specified `upload_path`.
    - Check that the server responds with the correct status codes for success and errors.

---

### **10. Additional Considerations**

- **Partial Uploads:**
    - Implement handling for incomplete uploads due to client disconnects or errors.

- **Non-Blocking I/O:**
    - Ensure that reading the request body and writing files are handled without blocking the server's event loop.
    - Use `poll()` to check when the socket is ready for reading.

- **Concurrency:**
    - Manage concurrent uploads and access to shared resources safely.

- **Logging:**
    - Implement logging to track file uploads and any errors that occur.

---
