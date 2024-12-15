#!/bin/bash

echo "Content-Type: text/plain"
echo ""
echo "CGI Tester"
echo "Environment variables:"
env | sort
echo ""
echo "POST data:"
cat -