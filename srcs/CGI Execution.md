# CGI Execution

### **Implementation Steps**

#### **1. Detecting CGI Requests**

First, determine if an incoming HTTP request should be handled by a CGI script based on:

- The request method (usually `GET` or `POST`).
- The requested URI and its file extension (e.g., `.php`, `.py`).
- Configuration settings that map certain routes or extensions to CGI handling.

**Example:**

```cpp
// In RequestHandler.cpp

bool RequestHandler::isCGIRequest(const HTTPRequest& request, const LocationConfig& location) {
    // Extract the file extension from the URI
    std::string uri = request.getURI();
    size_t dotPos = uri.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = uri.substr(dotPos);
        // Check if the extension is configured for CGI handling
        if (location.cgi_extensions.find(extension) != location.cgi_extensions.end()) {
            return true;
        }
    }
    return false;
}
```

---

#### **2. Setting Up Environment Variables**

When executing a CGI script, certain environment variables need to be set to provide the script with request information.

**Common Environment Variables:**

- `REQUEST_METHOD`
- `QUERY_STRING`
- `CONTENT_TYPE`
- `CONTENT_LENGTH`
- `SCRIPT_FILENAME` (the path to the CGI script)
- `SERVER_PROTOCOL`
- `PATH_INFO`

**Example:**

```cpp
// In CGIHandler.cpp

char** CGIHandler::createEnvironment(const HTTPRequest& request, const std::string& scriptPath) {
    std::map<std::string, std::string> env;

    env["REQUEST_METHOD"] = request.getMethod();
    env["QUERY_STRING"] = request.getQueryString();
    env["CONTENT_TYPE"] = request.getHeader("Content-Type");
    env["CONTENT_LENGTH"] = request.getHeader("Content-Length");
    env["SCRIPT_FILENAME"] = scriptPath;
    env["SERVER_PROTOCOL"] = request.getVersion();
    env["PATH_INFO"] = request.getURI();

    // Convert env map to char** array
    char** envp = new char*[env.size() + 1];
    size_t index = 0;
    for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); ++it) {
        std::string entry = it->first + "=" + it->second;
        envp[index] = new char[entry.size() + 1];
        strcpy(envp[index], entry.c_str());
        ++index;
    }
    envp[index] = NULL;
    return envp;
}
```

---

#### **3. Creating Pipes for Communication**

You'll need to set up pipes to communicate between the server and the CGI script:

- **Input Pipe:** For sending the request body to the CGI script (e.g., POST data).
- **Output Pipe:** For reading the CGI script's output to send back to the client.

**Example:**

```cpp
int inputPipe[2];  // For CGI stdin
int outputPipe[2]; // For CGI stdout

// Create pipes
if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0) {
    perror("pipe");
    // Handle error
}
```

---

#### **4. Forking and Executing the CGI Script**

Use `fork()` to create a child process in which you'll execute the CGI script using `execve()`.

**Example:**

```cpp
pid_t pid = fork();
if (pid < 0) {
    perror("fork");
    // Handle error
} else if (pid == 0) {
    // Child process

    // Redirect stdin and stdout
    dup2(inputPipe[0], STDIN_FILENO);   // Read from input pipe
    dup2(outputPipe[1], STDOUT_FILENO); // Write to output pipe

    // Close unused pipe ends
    close(inputPipe[1]);
    close(outputPipe[0]);

    // Prepare arguments and environment
    char* argv[] = { NULL }; // The script path is passed via SCRIPT_FILENAME
    char** envp = createEnvironment(request, scriptPath);

    // Execute the CGI script
    execve(cgiScriptPath.c_str(), argv, envp);

    // If execve fails
    perror("execve");
    exit(EXIT_FAILURE);
} else {
    // Parent process

    // Close unused pipe ends
    close(inputPipe[0]);
    close(outputPipe[1]);

    // Write request body to CGI's stdin if required
    if (request.getMethod() == "POST") {
        std::string body = request.getBody();
        write(inputPipe[1], body.c_str(), body.size());
    }
    close(inputPipe[1]); // Signal EOF to CGI script

    // Read CGI output
    char buffer[4096];
    std::string cgiOutput;
    int bytesRead;
    while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0) {
        cgiOutput.append(buffer, bytesRead);
    }
    close(outputPipe[0]);

    // Wait for the CGI process to finish
    int status;
    waitpid(pid, &status, 0);

    // Process CGI output and send response to client
}
```

---

#### **5. Processing CGI Output**

The output from the CGI script may include HTTP headers and body. You need to parse this output to construct the response to the client.

**Example:**

```cpp
// In handleCGIResponse function

size_t headerEnd = cgiOutput.find("\r\n\r\n");
if (headerEnd != std::string::npos) {
    std::string cgiHeaders = cgiOutput.substr(0, headerEnd);
    std::string cgiBody = cgiOutput.substr(headerEnd + 4);

    // Parse CGI headers
    std::map<std::string, std::string> headers;
    std::istringstream headerStream(cgiHeaders);
    std::string line;
    while (std::getline(headerStream, line) && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string headerName = line.substr(0, colonPos);
            std::string headerValue = line.substr(colonPos + 1);
            headers[headerName] = headerValue;
        }
    }

    // Construct the HTTP response
    Response response;
    response.setStatusCode(200);
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it) {
        response.setHeader(it->first, it->second);
    }
    response.setBody(cgiBody);

    // Send the response to the client
    std::string responseStr = response.toString();
    send(clientFd, responseStr.c_str(), responseStr.size(), 0);
} else {
    // Handle error: Invalid CGI output
}
```

---

#### **6. Handling Chunked Transfer Encoding**

According to the requirements, for chunked requests:

- The server needs to **unchunk** the incoming request body before passing it to the CGI script, which expects the end of the body at EOF.
- For the output of the CGI, if no `Content-Length` is returned, EOF will mark the end of the data.

**Unchunking Example:**

```cpp
std::string unchunkRequestBody(const std::string& chunkedBody) {
    std::istringstream stream(chunkedBody);
    std::string unchunkedBody;
    std::string line;

    while (std::getline(stream, line)) {
        size_t chunkSize = strtol(line.c_str(), NULL, 16);
        if (chunkSize == 0) {
            break; // End of chunks
        }
        char* buffer = new char[chunkSize];
        stream.read(buffer, chunkSize);
        unchunkedBody.append(buffer, chunkSize);
        delete[] buffer;
        // Consume the trailing \r\n after chunk data
        stream.ignore(2);
    }
    return unchunkedBody;
}
```

---

#### **7. Ensuring Non-Blocking I/O**

All I/O operations must be non-blocking and go through `poll()` (or equivalent):

- Set the pipes to non-blocking mode using `fcntl()`.
- Before reading from or writing to the pipes, use `poll()` to check if they are ready.

**Example:**

```cpp
// Set pipes to non-blocking
fcntl(outputPipe[0], F_SETFL, O_NONBLOCK);
fcntl(inputPipe[1], F_SETFL, O_NONBLOCK);

// Use poll to monitor the pipes
struct pollfd fds[2];
fds[0].fd = outputPipe[0];
fds[0].events = POLLIN;
fds[1].fd = inputPipe[1];
fds[1].events = POLLOUT;

int timeout = 5000; // Timeout in milliseconds
int ret = poll(fds, 2, timeout);
if (ret > 0) {
    if (fds[0].revents & POLLIN) {
        // Read from CGI output
    }
    if (fds[1].revents & POLLOUT) {
        // Write to CGI input
    }
}
```

---

#### **8. Updating Configuration for CGI**

Ensure that the configuration parser supports CGI settings:

- Define which file extensions trigger CGI execution.
- Map extensions to CGI executables.

**Example Configuration:**

```
server {
    listen 8080;
    server_name example.com;

    location /cgi-bin/ {
        root /var/www/cgi-bin;
        cgi_extensions .py .pl;
        cgi_bin /usr/bin/python;
    }
}
```

- In the `LocationConfig` structure, include a map of CGI extensions to executables.

---

#### **9. Updating the Request Handler**

Integrate CGI execution into the request handling flow:

- **Determine if the request is for CGI.**
- **Prepare the script path and executable.**
- **Execute the CGI script as outlined above.**

**Example:**

```cpp
void RequestHandler::handleRequest(const std::string& requestStr, int clientFd) {
    HTTPRequest request(requestStr);

    // Find matching server and location configurations
    const ServerConfig& serverConfig = findServerConfig(request);
    const LocationConfig& location = findLocationConfig(request, serverConfig);

    if (isCGIRequest(request, location)) {
        handleCGIRequest(request, clientFd, location);
    } else {
        // Handle as static content or other methods
    }
}
```

---

#### **10. Error Handling**

- **CGI Execution Errors:** If executing the CGI script fails, send a `500 Internal Server Error` response.
- **Timeouts:** Implement timeouts for CGI script execution to prevent the server from hanging.
- **Security:** Validate all inputs to prevent security issues like command injection.

---

### **Key Considerations**

- **Security:**
    - Ensure that the scripts executed are only those intended by the configuration.
    - Set appropriate permissions for CGI directories and scripts.
- **Resource Management:**
    - Avoid zombie processes by properly handling child processes (`waitpid`).
    - Clean up dynamically allocated memory to prevent leaks.
- **Compliance:**
    - Ensure that the CGI execution follows the specifications and handles edge cases.
    - Test with different types of CGI scripts (e.g., Python, PHP).

---

### **Testing CGI Execution**

- **Create Sample CGI Scripts:**
    - Write simple scripts in languages like Python or Perl that output HTTP headers and content.
- **Configure the Server:**
    - Update the configuration files to map the scripts' extensions to the CGI handler.
- **Send Test Requests:**
    - Use a browser or tools like `curl` to make requests to the CGI endpoints.
- **Verify Responses:**
    - Ensure that the server correctly executes the scripts and returns the expected output.

---

### **Next Steps**

- **Implement Timeouts and Limits:**
    - Prevent long-running scripts from impacting server performance.
- **Logging and Monitoring:**
    - Log CGI execution details for debugging and monitoring.
- **Additional Features:**
    - Support more CGI extensions as needed.
    - Implement more robust parsing of CGI output.

---
