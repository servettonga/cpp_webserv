# Non-blocking Server Implementation Notes

## Design Features
1. Single-threaded event loop using select()
2. Non-blocking I/O for all sockets
3. Buffer management for partial reads/writes
4. Connection state tracking
5. HTTP/1.1 support with keep-alive

## Performance Considerations
1. Efficient file descriptor management
2. Minimal memory allocation
3. Optimal buffer sizes
4. Quick connection cleanup

## Error Handling
1. Socket errors
2. Buffer overflows
3. Invalid requests
4. Resource exhaustion
5. Client disconnects

## Security Considerations
1. Request size limits
2. Header count limits
3. Path traversal prevention
4. Buffer overflow prevention

## Example Usage
```c++
int main() {
    try {
        // Create server on port 8080
        Server server(8080);
        
        // Start server (blocks until stop() called)
        server.start();
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```
