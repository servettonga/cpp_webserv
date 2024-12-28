#!/bin/bash

# Create logs directory and set permissions
mkdir -p logs
chmod 755 logs

# Ensure monitor_memory.sh is executable
chmod +x monitor_memory.sh

# Check if server is running
if ! nc -z localhost 8080; then
    echo "Error: Web server is not running on port 8080"
    echo "Please start the server first with: ./webserv config/default.conf"
    exit 1
fi

# Start monitoring
./monitor_memory.sh &
monitor_pid=$!

# Run siege test with correct URL format
echo "Starting siege test..."
siege -b -t60S http://127.0.0.1:8080/empty.html

# Cleanup
if [ ! -z "$monitor_pid" ]; then
    kill $monitor_pid 2>/dev/null || true
fi

# Analyze results
echo "Analyzing memory usage..."
if [ -f logs/memory_log.csv ]; then
    awk -F',' '{
        if(NR>1) {
            if($2 > max) max=$2
            if(min=="" || $2 < min) min=$2
            sum+=$2; count++
        }
    }
    END {
        print "Min Memory: "min" KB"
        print "Max Memory: "max" KB"
        print "Avg Memory: "sum/count" KB"
    }' logs/memory_log.csv
else
    echo "No memory log found at logs/memory_log.csv"
fi
