#!/bin/bash

while true; do
    echo "Active connections:"
    netstat -an | grep :8080 | wc -l
    sleep 1
done
