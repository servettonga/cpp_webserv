#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Configuration
SERVER="http://localhost:8080"
UPLOAD_PATH="/files/uploads"
SMALL_FILE="small_1kb.txt"
MEDIUM_FILE="medium_5mb.txt"
LARGE_FILE="large_15mb.txt"

# Function to print test result
test_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}[✓] $2${NC}"
    else
        echo -e "${RED}[✗] $2${NC}"
        echo -e "${RED}Response: $3${NC}"
        exit 1
    fi
}

# Create test files
echo "Creating test files..."
dd if=/dev/zero of="$SMALL_FILE" bs=1K count=1 2>/dev/null
dd if=/dev/zero of="$MEDIUM_FILE" bs=1M count=5 2>/dev/null
dd if=/dev/zero of="$LARGE_FILE" bs=1M count=15 2>/dev/null

# Test GET requests
echo -e "\nTesting GET request to root..."
curl -s "$SERVER/" > /dev/null
test_result $? "GET request to root"

echo -e "\nTesting GET request to uploads directory..."
curl -s "$SERVER$UPLOAD_PATH" > /dev/null
test_result $? "GET request to uploads directory"

# Test uploads with different file sizes
echo -e "\nTesting small file upload (1KB)..."
RESPONSE=$(curl -s -X POST -F "file=@$SMALL_FILE" "$SERVER$UPLOAD_PATH")
test_result $? "Small file upload" "$RESPONSE"

echo -e "\nTesting medium file upload (5MB)..."
RESPONSE=$(curl -s -X POST -F "file=@$MEDIUM_FILE" "$SERVER$UPLOAD_PATH")
test_result $? "Medium file upload" "$RESPONSE"

echo -e "\nTesting large file upload (15MB)..."
RESPONSE=$(curl -s -X POST -F "file=@$LARGE_FILE" "$SERVER$UPLOAD_PATH")
HTTP_CODE=$(curl -s -w "%{http_code}" -X POST -F "file=@$LARGE_FILE" "$SERVER$UPLOAD_PATH" -o /dev/null)
if [ "$HTTP_CODE" = "413" ]; then
    test_result 0 "Large file correctly rejected (413)" "$RESPONSE"
else
    test_result 1 "Large file should be rejected" "Got HTTP $HTTP_CODE: $RESPONSE"
fi

# Test DELETE for uploaded files
echo -e "\nTesting DELETE requests..."
for file in "$SMALL_FILE" "$MEDIUM_FILE"; do
    RESPONSE=$(curl -s -X DELETE "$SERVER$UPLOAD_PATH/$file")
    test_result $? "DELETE request for $file" "$RESPONSE"

    # Verify deletion
    HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "$SERVER$UPLOAD_PATH/$file")
    if [ "$HTTP_CODE" = "404" ]; then
        test_result 0 "Verified deletion of $file"
    else
        test_result 1 "File $file still exists" "HTTP $HTTP_CODE"
    fi
done

# Cleanup
echo -e "\nCleaning up test files..."
rm -f "$SMALL_FILE" "$MEDIUM_FILE" "$LARGE_FILE"

echo -e "\nAll tests completed!"