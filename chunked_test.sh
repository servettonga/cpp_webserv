#!/bin/bash

echo "=== Starting Chunked Transfer Test ==="
echo "Time: $(date)"
echo "Test size: 95.3 MB"

# Generate test data
echo "Generating test data..."
perl -e 'print "a" x 100000000' > test_input

# Get memory in MB
get_memory() {
    ps -o rss= -p $(pgrep webserv) | awk '{printf "%.2f", $1/1024}'
}

# Monitor peak memory
monitor_memory() {
    local peak=0
    while true; do
        local current=$(ps -o rss= -p $(pgrep webserv) | awk '{print $1}')
        if [ "$current" -gt "$peak" ]; then
            peak=$current
        fi
        sleep 0.1
    done
}

initial_memory=$(get_memory)
echo "Initial server memory: ${initial_memory} MB"

# Start memory monitoring in background
monitor_memory &
monitor_pid=$!

# Send request and measure time
echo "Sending chunked request..."
start_time=$(date +%s)

cat test_input | curl -v -X POST \
    -H "Transfer-Encoding: chunked" \
    -H "Content-Type: test/file" \
    -H "X-Secret-Header-For-Test: 1" \
    -H "Accept-Encoding: gzip" \
    -H "User-Agent: Go-http-client/1.1" \
    --data-binary @- \
    http://localhost:8080/directory/youpi.bla > test_output 2>test_headers

end_time=$(date +%s)
kill $monitor_pid

# Calculate results using awk
final_memory=$(get_memory)
duration=$((end_time - start_time))
transfer_rate=$(awk "BEGIN {printf \"%.2f\", 95.3 / $duration}")
memory_diff=$(awk "BEGIN {printf \"%.2f\", $final_memory - $initial_memory}")

echo -e "\n=== Test Results ==="
echo "Duration: $duration seconds"
echo "Transfer rate: $transfer_rate MB/s"
echo "Initial memory: $initial_memory MB"
echo "Final memory: $final_memory MB"
echo "Memory difference: $memory_diff MB"

# Verify response
input_size=$(wc -c < test_input)
output_size=$(wc -c < test_output)
echo -e "\nVerification:"
echo "Input size: $input_size bytes"
echo "Output size: $output_size bytes"

# Show first/last content
echo -e "\nInput content:"
echo "First bytes: $(head -c 50 test_input | xxd)"
echo "Last bytes: $(tail -c 50 test_input | xxd)"

echo -e "\nOutput content:"
echo "First bytes: $(head -c 50 test_output | xxd)"
echo "Last bytes: $(tail -c 50 test_output | xxd)"

# Show response headers
echo -e "\nResponse headers:"
grep "< " test_headers

# Cleanup
rm test_headers

exit 0