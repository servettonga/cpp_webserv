#!/usr/bin/env python3
import cgi
import os

print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>Python CGI Test</h1>")

# Print environment variables
print("<h2>Environment Variables:</h2>")
print("<ul>")
for key, value in os.environ.items():
    print(f"<li>{key}: {value}</li>")
print("</ul>")

# Handle form data
form = cgi.FieldStorage()
if form:
    print("<h2>Form Data:</h2>")
    print("<ul>")
    for key in form.keys():
        print(f"<li>{key}: {form[key].value}</li>")
    print("</ul>")

print("</body></html>")