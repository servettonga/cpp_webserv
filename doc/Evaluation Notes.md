## **HTTP Server Basics**

This HTTP server implementation follows standard `HTTP/1.1` protocol with key components:
- Socket initialization with non-blocking I/O
- Request parsing and validation
- Response generation with proper headers
- Static file serving and directory listing
- Error handling with appropriate status codes

### **I/O Multiplexing Function**

The project uses `select()` for I/O multiplexing as shown in the implementation:
```cpp
bool ServerGroup::handleSelect() {
    fd_set readSet = _masterSet;
    fd_set writeSet = _writeSet;

    timeval timeout = {SELECT_TIMEOUT_SEC, SELECT_TIMEOUT_USEC};
    int activity = select(_maxFd + 1, &readSet, &writeSet, NULL, &timeout);

    if (activity < 0) {
        if (errno == EINTR)
            return false;
        throw std::runtime_error("Select failed: " + std::string(strerror(errno)));
    }
    if (activity > 0)
        handleEvents(readSet, writeSet);
    return true;
}
```

### **`select()` Operation**

`select()` operates by monitoring file descriptors for three conditions:
- Read readiness (`readSet`)
- Write readiness (`writeSet`)
- Error conditions (not used in our case)

It blocks until activity is detected on any monitored descriptor or timeout occurs.

### **Single `select()` Management**

One `select()` handles both server and client operations:
- Server socket is monitored for new connections
- Client sockets are monitored for both read/write operations simultaneously
- File descriptors are managed in readSet and writeSet:
```cpp
void ServerGroup::handleEvents(fd_set& readSet, fd_set& writeSet) {
    // Handle all server events first (new connections)
    for (std::vector<Server*>::iterator it = _servers.begin();
         it != _servers.end(); ++it) {
        Server* server = *it;
        if (FD_ISSET(server->getServerSocket(), &readSet))
            server->handleNewConnection();
    }
    // Then handle existing client connections
    for (std::vector<Server*>::iterator it = _servers.begin();
         it != _servers.end(); ++it) {
        (*it)->handleExistingConnections(readSet, writeSet);
    }
}
```

### **Read/Write Error Handling**
Error handling follows these rules:
- Both `-1` and `0` return values are checked
- Clients are removed on any error condition:

```cpp
void Server::handleClientData(int clientFd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0)  // Connection closed by client
            std::cout << "Client disconnected: " << clientFd << std::endl;
        closeConnection(clientFd);  // Handle both -1 and 0 cases
        return;
    }
    // Process data...
}
```

- No errno checks after I/O operations
```cpp
ssize_t bytesRead = recv(fd, buffer, size, 0);
if (bytesRead <= 0) {
    // Handle error through return value
    closeConnection(fd);
    return;
}
```
- All I/O operations go through `select()`

#### Key Points:
- Uses `select()` for I/O multiplexing
- One `select()` handles both server and client operations
- Proper error handling without `errno` checks
- Non-blocking socket operations
- Clean separation of server and client handling logic

---

## Configuration

**HTTP Status Codes**:
```cpp
void Response::makeErrorResponse(int statusCode) {
    // Proper HTTP status codes implementation
    errorMessages[400] = "Bad Request";
    errorMessages[401] = "Unauthorized";
    errorMessages[403] = "Forbidden";
    errorMessages[404] = "Not Found";
    errorMessages[405] = "Method Not Allowed";
    errorMessages[413] = "Request Entity Too Large";
    errorMessages[500] = "Internal Server Error";
    errorMessages[501] = "Not Implemented";
}
```

**Multiple Servers Configuration**:
```conf
server {
    host 127.0.0.1;
    port 8080;
    // First server config
}

server {
    host 127.0.0.1;
    port 8081;
    // Second server config
}
```

**Virtual Hosts**:
```conf
server {
    host 127.0.0.1;
    port 8080;
    server_name localhost webserv.local;
}
```

**Custom Error Pages**:
```conf
    error_page 404 /errors/404.html;
    error_page 403 /errors/403.html;
    error_page 500 502 503 504 /errors/50x.html;
```

**Client Body Limits**:
```conf
    client_max_body_size 10M;  // Global limit

    location /upload {
        client_max_body_size 200M;  // Route-specific limit
    }
```

**Directory Routes**:
```conf
    location /static {
        root www;
        allowed_methods GET;
    }

    location /upload {
        root www/uploads;
        allowed_methods POST;
    }
```

**Default Files**:
```conf
    index index.html index.htm;  // Global default

    location / {
        index index.html;  // Location-specific default
    }
```

**Method Restrictions**:
```conf
    location / {
        allowed_methods GET POST;
    }

    location /upload {
        allowed_methods GET POST DELETE;
    }
```

All required configuration features are properly implemented and can be verified in `default.conf` and `test_server.conf`.


---

## Basic checks

**Example Tests Using curl:**

```bash
# GET Request
curl -v http://localhost:8080/

# File Upload
curl -v -X POST -F "file=@testfile.txt" http://localhost:8080/upload/

# File Retrieval
curl -v http://localhost:8080/upload/testfile.txt

# File Deletion
curl -v -X DELETE http://localhost:8080/upload/testfile.txt

# Unknown Method
curl -v -X INVALID http://localhost:8080/
```

**Code Implementation Reference:**

```cpp
Response RequestHandler::handleRequest(const Request &request) {
    // Method handling
    if (request.getMethod() == "GET") {
        response = handleGET(request);
    } else if (request.getMethod() == "POST") {
        response = handlePOST(request);
    } else if (request.getMethod() == "DELETE") {
        response = handleDELETE(request);
    } else {
        response = Response::makeErrorResponse(501); // Not Implemented
    }
    return response;
}
```

**Status Code Verification:**
```cpp
void Response::makeErrorResponse(int statusCode) {
    errorMessages[400] = "Bad Request";
    errorMessages[401] = "Unauthorized";
    errorMessages[403] = "Forbidden";
    errorMessages[404] = "Not Found";
    errorMessages[405] = "Method Not Allowed";
    errorMessages[413] = "Request Entity Too Large";
    errorMessages[500] = "Internal Server Error";
    errorMessages[501] = "Not Implemented";
}
```

---

## CGI

**Basic CGI Setup Verification**
```bash
# Test Python CGI
curl -v http://localhost:8080/cgi-bin/test.py
# Test PHP CGI
curl -v http://localhost:8080/cgi-bin/test.php
```

**Directory and Path Handling**
```cpp
void CGIHandler::setupEnvironment(const Request &request, const std::string &scriptPath) {
    _envMap["PATH_INFO"] = request.getPath();
    _envMap["PATH_TRANSLATED"] = scriptPath;
    _envMap["SCRIPT_FILENAME"] = scriptPath;
    // Working directory setup for relative paths
    chdir(_cwd.c_str());
}
```

**`GET` Request Testing**
```bash
# Test with query parameters
curl -v "http://localhost:8080/cgi-bin/test.py?name=test&data=123"
```

**`POST` Request Testing**
```bash
# Test form submission
curl -v -X POST \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "name=test&message=hello" \
    http://localhost:8080/cgi-bin/test.py
```

**Error Handling Tests**
```cpp
Response CGIHandler::executeCGI(const Request& request,
                              const std::string& cgiPath,
                              const std::string& scriptPath) {
    // Process timeout handling
    if (time(NULL) - start_time > CGI_TIMEOUT) {
        kill(pid, SIGTERM);
        return createErrorResponse(504, "CGI Timeout");
    }

    // Process exit status check
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return createErrorResponse(500, "CGI process failed");
    }
}
```

**Test Scripts for Error Cases**:
```python
#!/usr/bin/env python3
import time
# Infinite loop test
while True:
    time.sleep(1)
```

```python
#!/usr/bin/env python3
# Deliberate error
raise Exception("Test error")
```

### Key Points:
- Proper working directory handling
- Timeout handling
- Process management
- Error propagation
- Clean environment setup
- Secure execution

All error cases return appropriate HTTP status codes without crashing the server.

----

## Browser Test


**Test Setup Plan**
```markdown
1. Browser Network Tools Setup:
   - Open browser dev tools (F12)
   - Go to Network tab
   - Enable "Preserve log"

2. Test Cases to Execute:
   - Basic static file serving
   - Directory listing
   - Error pages
   - Redirects
   - Invalid URLs
```

**Test Execution Steps**
```markdown
A. Static Content Tests:
   1. Visit http://localhost:8080/
   2. Check HTML/CSS/image loading
   3. Verify response headers:
      - Content-Type
      - Content-Length
      - Connection
      - Server

B. Directory Tests:
   1. Visit http://localhost:8080/static/
   2. Verify directory listing if enabled
   3. Check index.html loading if present

C. Error Handling:
   1. Visit http://localhost:8080/nonexistent
   2. Verify 404 page
   3. Check error response headers

D. Redirect Tests:
   1. Visit configured redirect URL
   2. Verify 301/302 response
   3. Check Location header
```

**Implementation Reference**
```cpp
Response RequestHandler::handleRequest(const Request &request) {
    // Route matching
    const LocationConfig *location = getLocation(request.getPath());

    // Check redirects
    if (location && !location->redirect.empty()) {
        Response redirect(302);
        redirect.addHeader("Location", location->redirect);
        return redirect;
    }

    // Handle directory listing
    if (location && location->autoindex)
        return handleDirectoryListing(request.getPath());

    // Static file serving
    return handleStaticFile(request);
}
```

**Configuration Reference**
```conf
server {
    listen 8080;
    root /var/www;

    location / {
        index index.html;
    }

    location /static {
        autoindex on;
    }

    location /old {
        return 301 /new;
    }

    error_page 404 /404.html;
}
```

---

## Port issues

### **Port Validation**
```cpp
bool ConfigParser::validatePorts(const ServerConfig &config) const {
    return config.port > 0 && config.port < 65536;
}

bool ConfigParser::validate() {
    // Check for duplicate ports
    std::map<std::string, int> portMap;
    for (std::vector<ServerConfig>::iterator it = _configs.begin();
         it != _configs.end(); ++it) {
        std::string key = it->host + ":" + std::to_string(it->port);
        if (portMap[key]++) {
            addError("Duplicate port configuration for " + key);
            return false;
        }
    }
}
```

**Configuration Examples**

Valid multi-port configuration:
```nginx
server {
    host 127.0.0.1;
    port 8080;
    root www;
}

server {
    host 127.0.0.1;
    port 8081;
    root www/portfolio;
}
```

**Test Script**
```bash
test_ports() {
    # Test main server
    curl -s "http://localhost:8080/" > /dev/null
    test_result $? "Main server on port 8080"

    # Test secondary server
    curl -s "http://localhost:8081/" > /dev/null
    test_result $? "Portfolio server on port 8081"
}
```

### Key Points:
- Socket binding fails if port is already in use
- Only first server for host:port pair becomes default
- Port range validation (1-65535)
- Port uniqueness check per host
- Error handling for bind failures

---

## Siege and Stress Test


**Basic Siege Test**
```bash
# Initial availability test (60 seconds)
siege -b -t60S http://localhost:8080/empty.html
```

**Memory Monitoring Script:**
```bash
#!/bin/bash

mkdir -p logs
echo "Starting memory monitoring..."
echo "Time,RSS(KB)" > logs/memory_log.csv

while true; do
    timestamp=$(date +%s)
    pid=$(pgrep webserv)
    if [ ! -z "$pid" ]; then
        memory=$(ps -o rss= -p $pid)
        echo "$timestamp,$memory" >> logs/memory_log.csv
    fi
    sleep 1
done
```

**Long Duration Test Script:**
```bash
#!/bin/bash

# Create logs directory and set permissions
mkdir -p logs
chmod 755 logs

# Ensure monitor_memory.sh is executable
chmod +x monitor_memory.sh

# Check if server is running
if ! nc -z localhost 8080; then
    echo "Error: Web server is not running on port 8080"
    echo "Please start the server first with: ./webserv config/default.conf"
    exit 1
fi

# Start monitoring
./monitor_memory.sh &
monitor_pid=$!

# Run siege test with correct URL format
echo "Starting siege test..."
siege -b -t60S http://127.0.0.1:8080/empty.html

# Cleanup
if [ ! -z "$monitor_pid" ]; then
    kill $monitor_pid 2>/dev/null || true
fi

# Analyze results
echo "Analyzing memory usage..."
if [ -f logs/memory_log.csv ]; then
    awk -F',' '{
        if(NR>1) {
            if($2 > max) max=$2
            if(min=="" || $2 < min) min=$2
            sum+=$2; count++
        }
    }
    END {
        print "Min Memory: "min" KB"
        print "Max Memory: "max" KB"
        print "Avg Memory: "sum/count" KB"
    }' logs/memory_log.csv
else
    echo "No memory log found at logs/memory_log.csv"
fi
```

**Test Execution Order:**
```markdown
1. Start server
2. Start connection monitor
3. Run initial siege test
4. Check availability percentage
5. Start memory monitor
6. Run extended siege test
7. Analyze results
```

**Expected Results:**
```markdown
- Availability > 99.5%
- Memory usage stable after initial spike
- No hanging connections
- Server remains responsive
```

The implementation should show:
- Proper connection cleanup
- Memory stability
- High availability
- No resource leaks

---

## Cookies and Session

**Test Components:**
```markdown
1. Cookie Setting & Reading
2. Session Creation & Maintenance
3. Session Expiry
4. Multiple Session Handling
```

```bash
#!/bin/bash

echo "Testing Cookie & Session System..."

# Test 1: Basic Cookie Setting
curl -v -c cookies.txt http://localhost:8080/

# Test 2: Session Persistence
curl -v -b cookies.txt http://localhost:8080/cookie-test.html

# Test 3: Multiple Sessions
curl -v -c cookies2.txt http://localhost:8080/static
curl -v -b cookies2.txt http://localhost:8080/upload

# Test 4: Session Timeout
sleep 60
curl -v -b cookies.txt http://localhost:8080/cookie-test.html


# Cleanup
rm -f cookies.txt cookies2.txt
```

```cpp
class Response {
    void setCookie(const std::string& name, const std::string& value);
    std::string getCookie(const std::string& name);
    void createSession();
    bool validateSession();
};
```

Expected output shows:
- Cookie setting in headers
- Session ID creation
- Session persistence
- Proper timeout handling
- Clean session cleanup

---

## Multiple CGI Handler

**Configuration Verification**
```nginx
server {
    # CGI Configuration
    cgi {
        .py     /usr/bin/python3;
        .php    www/cgi-bin/php/bin/php-cgi;
        .bla    /test_server/cgi-bin/cgi-tester;
    }

    location ~ \.(py|php|bla)$ {
        root www/cgi-bin;
        allowed_methods GET POST;
    }
}
```

**Test Commands**
```bash
# Test Python CGI
curl -X POST -d "data=test" http://localhost:8080/cgi-bin/test.py

# Test PHP CGI
curl -X POST -d "data=test" http://localhost:8080/cgi-bin/test.php

# Test .bla CGI
curl -X POST -d "data=test" http://localhost:8080/directory/youpi.bla
```

### Key Points:

- Multiple CGI handler support
- Different CGI types processing
- Proper environment setup per CGI type
- Error handling for each type
