# CGIHandler Implementation Notes

## Key Requirements
1. Execute CGI scripts based on file extension
2. Set up proper environment variables
3. Handle chunked request data
4. Process CGI output correctly
5. Support multiple CGI types

## Error Handling
1. Script not found
2. Permission denied
3. Execution failures
4. Timeout handling
5. Resource cleanup

## Security Considerations
1. Path validation
2. Script permissions
3. Resource limits
4. Working directory security
5. Environment sanitization

## Testing Scenarios
1. Basic script execution
2. Large input handling
3. Chunked data
4. Various CGI types
5. Error conditions

## Example usage of CGIHandler
```c++
void handleCGIRequest(const HTTPRequest &request, const std::string &scriptPath) {
    /*
    1. Create CGIHandler instance
    2. Execute CGI script
    3. Get response
    4. Send response to client
    */
}
```
