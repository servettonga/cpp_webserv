# A HTTP server in C++98

A lightweight HTTP/1.1 compliant web server supporting static file serving, CGI execution, and virtual hosts.

## Features

- **HTTP/1.1 Compliance**
  - `GET`, `POST`, `DELETE` methods
  - Keep-alive connections
  - Status codes per RFC standards

- **Core Functionality**
  - Static file serving
  - Directory listing
  - File uploads
  - CGI execution
  - Virtual host support

- **Performance**
  - Non-blocking I/O
  - Single `select()` multiplexing
  - Connection pooling
  - Memory-efficient design

- **Configuration**
  - Multiple port binding
  - Custom error pages
  - Route-based rules
  - Directory restrictions
  - Upload size limits

## Architecture

```mermaid
sequenceDiagram
    participant Client
    participant Listener as Server Listener
    participant Poller as Polling Mechanism
    participant Handler as Request Handler
    participant Router
    participant Methods as HTTP Methods
    participant Static as Static File Server
    participant CGI as CGI Handler
    participant Error as Error Handler
    participant Response as Response Generator

    Client->>Listener: Connect to Server
    Listener->>Poller: Register Socket
    Poller-->>Listener: Socket Ready
    Listener->>Handler: Accept Connection
    Handler->>Poller: Register Client Socket
    Poller-->>Handler: Data Ready
    Handler->>Client: Read HTTP Request
    Handler->>Router: Parse Request
    Router->>Router: Match Route
    alt Request Type
        opt Static Content
            Router->>Static: Retrieve File
            Static-->>Handler: Serve File
        end
        opt Execute CGI
            Router->>CGI: Run Script
            CGI-->>Handler: CGI Output
        end
        opt Unsupported Method
            Router->>Error: Generate Error
            Error-->>Handler: Error Response
        end
    end
    Handler->>Response: Generate HTTP Response
    Response->>Client: Send Response
    Client-->>Handler: Acknowledgment
    Handler->>Poller: Remove Socket if Done
```

## Quick Start

```bash
# Build the server
make

# Run with default config
./webserv config/default.conf

# Test basic functionality
curl http://localhost:8080/
```

## Configuration Example

```nginx
server {
    listen 8080;
    server_name localhost;
    root /var/www;

    location / {
        index index.html;
        methods GET POST;
    }

    location /cgi-bin {
        cgi on;
        cgi_extensions .php .py;
    }
}
```

## Requirements

- C++98 compiler
- POSIX-compliant system
- CMake 3.0+ (for building)

---

This project is part of the 42 school curriculum.
