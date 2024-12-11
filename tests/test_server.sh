#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Server configurations
MAIN_SERVER="http://localhost:8080"
PORTFOLIO_SERVER="http://localhost:8081"
UPLOAD_PATH="/upload"

# Test files
SMALL_FILE="small_1kb.txt"
MEDIUM_FILE="medium_5mb.txt"
LARGE_FILE="large_25mb.txt"

# Function to print test result
test_result() {
    if [ "$1" -eq 0 ]; then
        echo -e "${GREEN}[✓] $2${NC}"
    else
        echo -e "${RED}[✗] $2${NC}"
        echo -e "${RED}Response: $3${NC}"
        exit 1
    fi
}

# Function to test server response with host header
test_virtual_host() {
    local url=$1
    local host=$2
    local expected_code=$3
    local message=$4

    HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -H "Host: $host" "$url")
    if [ "$HTTP_CODE" = "$expected_code" ]; then
        test_result 0 "$message"
    else
        test_result 1 "$message" "Expected $expected_code, got $HTTP_CODE"
    fi
}

# Create test files
echo "Creating test files..."
dd if=/dev/zero of="$SMALL_FILE" bs=1K count=1 2>/dev/null
dd if=/dev/zero of="$MEDIUM_FILE" bs=1M count=5 2>/dev/null
dd if=/dev/zero of="$LARGE_FILE" bs=1M count=25 2>/dev/null

# Test main server
echo -e "\nTesting main server (8080)..."
curl -s "$MAIN_SERVER/" > /dev/null
test_result $? "Main server GET request to root"

# Test portfolio server
echo -e "\nTesting portfolio server (8081)..."
curl -s "$PORTFOLIO_SERVER/" > /dev/null
test_result $? "Portfolio server GET request to root"

# Test virtual hosting
echo -e "\nTesting virtual hosting..."
test_virtual_host "$MAIN_SERVER/" "localhost" "200" "Default host on main server"
test_virtual_host "$PORTFOLIO_SERVER/" "portfolio.localhost" "200" "Portfolio virtual host"

# Test uploads on main server
echo -e "\nTesting uploads on main server..."
for file in "$SMALL_FILE" "$MEDIUM_FILE"; do
    echo "Attempting to upload $file..."
    # Get status code and response body separately
    RESPONSE=$(curl -s -w "\n%{http_code}" -X POST -F "file=@$file" "$MAIN_SERVER$UPLOAD_PATH")
    STATUS_CODE=$(echo "$RESPONSE" | tail -n1)

    # Check for successful upload (201 Created)
    if [ "$STATUS_CODE" = "201" ]; then
        test_result 0 "Upload of $file"

        # Verify upload
        echo "Verifying upload of $file..."
        HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "$MAIN_SERVER$UPLOAD_PATH/$file")
        if [ "$HTTP_CODE" = "200" ]; then
            test_result 0 "Verified upload of $file"
        else
            test_result 1 "File $file not uploaded" "HTTP $HTTP_CODE"
        fi
    else
        test_result 1 "Failed to upload $file" "HTTP $STATUS_CODE"
    fi
    sleep 1
done

# Test large file rejection
echo -e "\nTesting large file rejection..."
HTTP_CODE=$(curl -s -w "%{http_code}" -X POST -F "file=@$LARGE_FILE" "$MAIN_SERVER$UPLOAD_PATH" -o /dev/null)
if [ "$HTTP_CODE" = "413" ]; then
    test_result 0 "Large file correctly rejected (413)"
else
    test_result 1 "Large file should be rejected" "Got HTTP $HTTP_CODE"
fi

# Test DELETE on main server
echo -e "\nTesting DELETE requests on main server..."
echo -e "\nTesting DELETE requests on main server..."
for file in "$SMALL_FILE" "$MEDIUM_FILE"; do
    echo "Attempting to delete $file..."
    RESPONSE=$(curl -s --max-time 5 -X DELETE "$MAIN_SERVER$UPLOAD_PATH/$file")
    test_result $? "DELETE request for $file" "$RESPONSE"

    # Verify deletion with timeout
    echo "Verifying deletion of $file..."
    HTTP_CODE=$(curl -s --max-time 5 -o /dev/null -w "%{http_code}" "$MAIN_SERVER$UPLOAD_PATH/$file")
    if [ "$HTTP_CODE" = "404" ]; then
        test_result 0 "Verified deletion of $file"
    else
        test_result 1 "File $file still exists" "HTTP $HTTP_CODE"
    fi
    sleep 1
done

# Test method restrictions on portfolio server
echo -e "\nTesting method restrictions on portfolio server..."
HTTP_CODE=$(curl -s -X POST -w "%{http_code}" "$PORTFOLIO_SERVER/" -o /dev/null)
if [ "$HTTP_CODE" = "405" ]; then
    test_result 0 "POST correctly rejected on portfolio server"
else
    test_result 1 "POST should be rejected on portfolio server" "Got HTTP $HTTP_CODE"
fi

# Cleanup
echo -e "\nCleaning up test files..."
rm -f "$SMALL_FILE" "$MEDIUM_FILE" "$LARGE_FILE"

echo -e "\nAll tests completed!"