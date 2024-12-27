#!/bin/bash

echo "Testing Cookie & Session System..."

# Test 1: Basic Cookie Setting
curl -v -c cookies.txt http://localhost:8080/

# Test 2: Session Persistence
curl -v -b cookies.txt http://localhost:8080/cookie-test.html

# Test 3: Multiple Sessions
curl -v -c cookies2.txt http://localhost:8080/static
curl -v -b cookies2.txt http://localhost:8080/upload

# Test 4: Session Timeout
sleep 60
curl -v -b cookies.txt http://localhost:8080/cookie-test.html


# Cleanup
rm -f cookies.txt cookies2.txt
