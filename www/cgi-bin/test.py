#!/usr/bin/python3

import os
import sys
import urllib.parse

def parse_post_data(data):
    result = {}
    pairs = data.split('&')
    for pair in pairs:
        if '=' in pair:
            key, value = pair.split('=', 1)
            result[urllib.parse.unquote_plus(key)] = urllib.parse.unquote_plus(value)
    return result

def main():
    # First output required CGI response header
    print("Content-Type: text/html; charset=utf-8")
    print("Status: 200 OK")
    print()  # Empty line required between headers and body

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
            print("<ul>")
            for key, value in parse_post_data(post_data).items():
                print(f"<li><strong>{key}:</strong> {value}</li>")
            print("</ul>")

    print("</body>")
    print("</html>")
    sys.stdout.flush()

if __name__ == "__main__":
    if hasattr(sys.stdout, 'buffer'):
        sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', buffering=1)
    main()