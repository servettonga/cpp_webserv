#!/usr/bin/python3

import os
import sys

def main():
    # First output required CGI response header
    print("Content-Type: text/html; charset=utf-8")
    print("Status: 200 OK")
    print()  # Empty line required between headers and body

    # Start HTML output
    print("<html>")
    print("<head><title>Python CGI Test</title></head>")
    print("<body>")
    print("<h1>Python CGI Test</h1>")

    # Print environment variables
    print("<h2>Environment Variables:</h2>")
    print("<ul>")
    for key, value in sorted(os.environ.items()):
        print(f"<li><strong>{key}:</strong> {value}</li>")
    print("</ul>")

    # Handle POST data if present
    if os.environ.get('REQUEST_METHOD') == 'POST':
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        if content_length > 0:
            post_data = sys.stdin.buffer.read(content_length).decode('utf-8')
            print("<h2>POST Data:</h2>")
            print(f"<pre>{post_data}</pre>")

    print("</body>")
    print("</html>")

    # Ensure output is flushed
    sys.stdout.flush()

if __name__ == "__main__":
    # Ensure our output is properly buffered
    if hasattr(sys.stdout, 'buffer'):
        sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', buffering=1)
    main()