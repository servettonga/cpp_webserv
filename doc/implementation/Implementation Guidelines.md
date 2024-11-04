# Implementation Guidelines

## Core Components
- Server
- RequestHandler
- Response
- HTTPRequest
- CGIHandler
- ConfigParser
- Logger

## Core Components Implementation Order

1. HTTPRequest & Response
    - Start with these as they're needed by all other components
    - Implement basic parsing and generation first
    - Add advanced features later

2. ConfigParser
    - Implement basic config loading
    - Add validation
    - Support for virtual hosts

3. Server
    - Basic socket setup
    - Non-blocking I/O
    - Event loop with poll()

4. RequestHandler
    - Basic GET handling
    - Add POST and DELETE
    - Error handling

5. CGIHandler
    - Basic script execution
    - Environment setup
    - I/O handling

## Best Practices

### Memory Management
- Use RAII principles
- Avoid raw pointers
- Clean up resources in destructors
- Use smart containers for dynamic memory

### Error Handling
- Use exceptions for initialization errors
- Use return codes for runtime errors
- Log all errors with context
- Provide meaningful error messages

### Performance
- Non-blocking I/O everywhere
- Efficient buffer management
- Avoid copying large data
- Use references where possible

### Security
- Validate all inputs
- Sanitize file paths
- Check permissions
- Handle timeouts

## Server Component Pseudo Code

```cpp
class Server {
    /*
    MEMBERS:
    - Vector of listening sockets
    - Vector of server configurations
    - Map of client connections
    - Poll file descriptors
    - Request handler instance
    */

    /*
    MAIN OPERATIONS:
    1. Initialize server with config file
    2. Set up listening sockets
    3. Main event loop using poll()
    4. Accept new connections
    5. Handle client data
    6. Close connections
    */
}
```

## Request Handler Component Pseudo Code

```cpp
class RequestHandler {
    /*
    MEMBERS:
    - Server configurations
    - CGI handler instance
    - Supported methods map
    */

    /*
    MAIN OPERATIONS:
    1. Parse incoming requests
    2. Route requests to appropriate handlers
    3. Handle GET requests
    4. Handle POST requests
    5. Handle DELETE requests
    6. Execute CGI scripts when needed
    7. Generate responses
    */
}
```

## CGI Handler Component Pseudo Code

```cpp
class CGIHandler {
    /*
    MEMBERS:
    - Environment variables map
    - Working directory
    - Script path
    - Path info
    */

    /*
    MAIN OPERATIONS:
    1. Set up CGI environment
    2. Execute CGI scripts
    3. Handle input/output pipes
    4. Process chunked data
    5. Parse CGI output
    6. Generate response
    */
}
```

## Configuration Parser Component Pseudo Code

```cpp
class ConfigParser {
    /*
    MEMBERS:
    - Config file path
    - Server configurations
    */

    /*
    MAIN OPERATIONS:
    1. Parse configuration file
    2. Validate configuration
    3. Store server settings
    4. Handle virtual hosts
    5. Set up CGI configurations
    */
}
```
