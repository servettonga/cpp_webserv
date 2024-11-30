#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Configuration
SERVER="http://localhost:8080"
UPLOAD_PATH="/files/uploads"
TEST_FILE="test_file.txt"
TEST_CONTENT="This is a test file for webserver testing"

# Create test file
echo "Creating test file..."
echo "$TEST_CONTENT" > "$TEST_FILE"

# Function to print test result
test_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}[✓] $2${NC}"
    else
        echo -e "${RED}[✗] $2${NC}"
        exit 1
    fi
}

# Test GET - root
echo -e "\nTesting GET request to root..."
curl -s "$SERVER/" > /dev/null
test_result $? "GET request to root"

# Test GET - uploads directory
echo -e "\nTesting GET request to uploads directory..."
curl -s "$SERVER$UPLOAD_PATH" > /dev/null
test_result $? "GET request to uploads directory"

# Test POST - file upload
echo -e "\nTesting POST request (file upload)..."
UPLOAD_RESPONSE=$(curl -s -X POST -F "file=@$TEST_FILE" "$SERVER$UPLOAD_PATH")
test_result $? "POST request (file upload)"

# Verify file exists
echo -e "\nVerifying uploaded file..."
curl -s "$SERVER$UPLOAD_PATH/$TEST_FILE" > /dev/null
test_result $? "File exists check"

# Test DELETE
echo -e "\nTesting DELETE request..."
curl -s -X DELETE "$SERVER$UPLOAD_PATH/$TEST_FILE" > /dev/null
test_result $? "DELETE request"

# Verify file deleted
echo -e "\nVerifying file deletion..."
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "$SERVER$UPLOAD_PATH/$TEST_FILE")
if [ "$HTTP_CODE" = "404" ]; then
    test_result 0 "File successfully deleted"
else
    test_result 1 "File still exists"
fi

# Cleanup
rm -f "$TEST_FILE"

echo -e "\nAll tests completed!"