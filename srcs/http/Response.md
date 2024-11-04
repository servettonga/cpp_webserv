# Response Implementation Notes

## Status Codes
1. 2xx - Success
    - `200` OK
    - `201` Created
    - `204` No Content

2. 3xx - Redirection
    - `301` Moved Permanently
    - `302` Found
    - `304` Not Modified

3. 4xx - Client Error
    - `400` Bad Request
    - `403` Forbidden
    - `404` Not Found

4. 5xx - Server Error
    - `500` Internal Server Error
    - `502` Bad Gateway
    - `503` Service Unavailable

## Header Management
1. Common Headers
    - `Content-Type`
    - `Content-Length`
    - `Date`
    - `Server`
    - `Connection`

2. Special Headers
    - `Set-Cookie`
    - `Location`
    - `Cache-Control`
    - `ETag`

## Content Types
1. `text/html`
2. `text/plain`
3. `application/json`
4. `application/xml`
5. `application/octet-stream`

## Security Headers
1. `X-Content-Type-Options`
2. `X-Frame-Options`
3. `X-XSS-Protection`
4. `Content-Security-Policy`
5. `Strict-Transport-Security`

## Example usage of Response
```c++
   // Create response with specific status
   Response resp(404);
   std::cout << resp.getStatusCode();  // prints 404
   
   // Header manipulation
   resp.addHeader("Content-Type", "text/html");
   if (resp.hasHeader("content-type")) {  // case-insensitive
       std::cout << resp.getHeader("Content-Type");
   }
   resp.removeHeader("Content-Type");
   
   // Body handling
   resp.setBody("<h1>Not Found</h1>");
   resp.appendBody("<p>The requested resource was not found.</p>");
   std::cout << resp.getBody();
   
   // Complete response
   std::cout << resp.toString();
```

## Helper Methods

### Header Management
- Case-insensitive handling
- Special header tracking
- Validation of values
- Efficient storage

### Body Handling
- Content length tracking
- Content type detection
- Efficient appending
- Binary safety

### Formatting
- RFC compliance
- Performance optimization
- Consistent line endings
- Error prevention

## Testing Guide

### Test Cases
1. Basic Responses
    - `200 OK` with body
    - `404 Not Found`
    - `500 Server Error`

2. Headers
    - Standard headers
    - Custom headers
    - Multiple headers
    - Special characters

3. Content Types
    - HTML content
    - JSON content
    - Binary content
    - File content

4. Special Cases
    - Redirects
    - Cookies
    - Chunked encoding
    - Empty responses

5. Security
    - Security headers
    - XSS prevention
    - MIME type handling

## Example Response
```http request
HTTP/1.1 404 Not Found
Date: Mon, 04 Nov 2024 21:41:08 GMT
Server: webserv/1.0
Content-Type: text/html
Content-Length: 56
Connection: keep-alive

<h1>Not Found</h1><p>The requested resource was not found.</p>
```