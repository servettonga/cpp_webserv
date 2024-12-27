#!/bin/bash

mkdir -p logs
echo "Starting memory monitoring..."
echo "Time,RSS(KB)" > logs/memory_log.csv

while true; do
    timestamp=$(date +%s)
    pid=$(pgrep webserv)
    if [ ! -z "$pid" ]; then
        memory=$(ps -o rss= -p $pid)
        echo "$timestamp,$memory" >> logs/memory_log.csv
    fi
    sleep 1
done
