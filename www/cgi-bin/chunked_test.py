#!/usr/bin/python3

import os
import sys
import time
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
    # Send headers
    print("Content-Type: text/html; charset=utf-8")
    print("Status: 200 OK")
    print()
    sys.stdout.flush()

    chunks = [
        "<html><body><h1>Chunked Transfer Test</h1>",
        "<p>This is chunk 1 - Testing chunked response</p>",
        "<p>This is chunk 2 - With artificial delay</p>",
        "<h2>Environment Variables:</h2><ul>"
    ]

    # Add environment variables
    for key, value in sorted(os.environ.items()):
        chunks.append(f"<li><strong>{key}:</strong> {value}</li>")
    chunks.append("</ul>")

    # Handle POST data
    if os.environ.get('REQUEST_METHOD') == 'POST':
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        if content_length > 0:
            post_data = sys.stdin.buffer.read(content_length).decode('utf-8')
            chunks.append("<h2>POST Data:</h2><ul>")
            for key, value in parse_post_data(post_data).items():
                chunks.append(f"<li><strong>{key}:</strong> {value}</li>")
            chunks.append("</ul>")

    chunks.append("</body></html>")

    # Send chunks
    for chunk in chunks:
        print(chunk)
        sys.stdout.flush()
        time.sleep(0.5)

if __name__ == "__main__":
    if hasattr(sys.stdout, 'buffer'):
        sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', buffering=1)
    main()