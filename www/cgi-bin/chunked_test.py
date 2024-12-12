#!/usr/bin/env python3
import cgi
import os
import time
import sys

# Send headers
print("Content-Type: text/html")
print("Transfer-Encoding: chunked")
print()
sys.stdout.flush()

# Send data in chunks with delays
chunks = [
    "<html><body><h1>Chunked Transfer Test</h1>",
    "<p>This is chunk 1 - Testing chunked response</p>",
    "<p>This is chunk 2 - With artificial delay</p>",
    "<h2>Environment Variables:</h2><ul>",
]

# Add environment variables
env_vars = []
for key, value in os.environ.items():
    env_vars.append(f"<li>{key}: {value}</li>")
chunks.extend(env_vars)
chunks.append("</ul>")

# Handle form data if present
form = cgi.FieldStorage()
if form:
    chunks.append("<h2>Form Data:</h2><ul>")
    for key in form.keys():
        chunks.append(f"<li>{key}: {form[key].value}</li>")
    chunks.append("</ul>")

chunks.append("</body></html>")

# Send chunks with delay
for chunk in chunks:
    print(f"{len(chunk):X}")  # Size in hex
    print(chunk)
    print()  # Empty line after chunk
    sys.stdout.flush()
    time.sleep(0.5)  # Add delay between chunks

# Send final chunk
print("0")
print()
sys.stdout.flush()