# HTTPRequest Implementation Notes

## Parsing Rules
1. Request Line Format: METHOD URI HTTP/VERSION
2. Header Format: Name: Value
3. Body Parsing:
    - Raw body
    - URL-encoded form data
    - Multipart form data
    - Chunked transfer encoding

## Security Considerations
1. Buffer overflow prevention
2. Header injection prevention
3. Path traversal protection
4. Request size limits
5. Character encoding validation

## Supported Methods
1. `GET`
2. `POST`
3. `DELETE`
4. `HEAD` (optional)
5. `PUT` (optional)

## Content Types
1. `application/x-www-form-urlencoded`
2. `multipart/form-data`
3. `application/json`
4. `text/plain`
5. Custom types

## Error Cases
1. Invalid request line
2. Malformed headers
3. Invalid content length
4. Boundary not found
5. Invalid character encoding

## Example usage of HTTPRequest
```c++
void handleRawRequest(const std::string& rawData) {
    try {
        HTTPRequest request(rawData);
        
        // Access request components
        std::string method = request.getMethod();
        std::string uri = request.getURI();
        
        // Handle multipart form data
        if (request.getBoundary() != "") {
            std::vector<HTTPRequest::FormDataPart> parts = 
                request.parseMultipartFormData();
            // Process form parts
        }
        
    } catch (const std::exception& e) {
        // Handle parsing error
    }
}
```

## Example multipart form data
```http request
-----------------------------12345
Content-Disposition: form-data; name="field1"

value1
-----------------------------12345
Content-Disposition: form-data; name="file1"; filename="test.txt"
Content-Type: text/plain

file contents here
-----------------------------12345--

# Would parse into;
// First part
parts[0].headers = {
    {"Content-Disposition", "form-data; name=\"field1\""}
};
parts[0].content = "value1";

// Second part
parts[1].headers = {
    {"Content-Disposition", "form-data; name=\"file1\"; filename=\"test.txt\""},
    {"Content-Type", "text/plain"}
};
parts[1].content = "file contents here";
```



## Testing Guide

### Test Cases
1. Basic Requests
   - `GET` request
   - `POST` with form data
   - `DELETE` request

2. Headers
   - Multiple headers
   - Duplicate headers
   - Missing required headers
   - Invalid header format

3. Body Content
   - URL-encoded form data
   - Multipart form data
   - Chunked transfer encoding
   - Raw body data

4. Edge Cases
   - Empty request
   - Maximum size request
   - Invalid methods
   - Malformed request line
   - Missing line endings

5. Security Cases
   - Path traversal attempts
   - Header injection attempts
   - Oversize requests
   - Invalid encodings
