# Configuration Guide

## Server Configuration
```nginx
# Example server configuration
server {
    listen 80;
    server_name example.com;
    root /var/www;
    
    client_max_body_size 10M;
    
    location / {
        index index.html;
        methods GET POST DELETE;
    }
    
    location /cgi-bin {
        cgi on;
        cgi_extensions .php .py;
    }
}
```

## CGI Configuration

### Environment Setup
- PATH_INFO
- QUERY_STRING
- REQUEST_METHOD
- CONTENT_TYPE
- CONTENT_LENGTH
- SCRIPT_FILENAME

### Script Handling
- Permission checks
- Working directory
- Input/Output handling
- Error handling