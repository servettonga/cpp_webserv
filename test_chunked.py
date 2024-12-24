#!/usr/bin/python3

# Install required packages: pip install psutil requests

import os
import sys
import time
import psutil
import requests
from datetime import datetime

def generate_test_data(size):
    return 'a' * size

def monitor_memory(pid):
    try:
        process = psutil.Process(pid)
        return process.memory_info().rss / 1024 / 1024  # MB
    except:
        return 0

def test_chunked_transfer():
    TEST_SIZE = 100000000  # 100MB
    URL = "http://localhost:8080/directory/youpi.bla"
    CHUNK_SIZE = 65536     # 64KB chunks

    print(f"=== Starting Chunked Transfer Test ===")
    print(f"Time: {datetime.now()}")
    print(f"Test size: {TEST_SIZE/1024/1024:.1f} MB")
    print(f"Chunk size: {CHUNK_SIZE/1024:.1f} KB")

    # Generate test data
    print("\nGenerating test data...")
    data = generate_test_data(TEST_SIZE)

    # Find webserv process
    server_pid = None
    for proc in psutil.process_iter(['pid', 'name']):
        if 'webserv' in proc.info['name']:
            server_pid = proc.info['pid']
            break

    if not server_pid:
        print("Error: webserv process not found")
        sys.exit(1)

    # Setup request
    headers = {
        'Transfer-Encoding': 'chunked',
        'Content-Type': 'test/file',
        'X-Secret-Header-For-Test': '1'
    }

    # Monitor initial memory
    initial_memory = monitor_memory(server_pid)
    print(f"\nInitial server memory: {initial_memory:.1f} MB")

    # Send request
    print("\nSending chunked request...")
    start_time = time.time()
    max_memory = initial_memory

    try:
        with requests.post(URL, data=data, headers=headers, stream=True) as response:
            # Monitor memory during transfer
            current_memory = monitor_memory(server_pid)
            max_memory = max(max_memory, current_memory)

            # Get response
            response_data = response.content

            # Verify response
            print(f"\nResponse status: {response.status_code}")
            print(f"Response size: {len(response_data):,} bytes")
            print(f"Response headers:")
            for key, value in response.headers.items():
                print(f"  {key}: {value}")

    except Exception as e:
        print(f"Error during request: {e}")
        sys.exit(1)

    # Final statistics
    end_time = time.time()
    final_memory = monitor_memory(server_pid)

    print(f"\n=== Test Results ===")
    print(f"Total time: {end_time - start_time:.2f} seconds")
    print(f"Final server memory: {final_memory:.1f} MB")
    print(f"Peak memory usage: {max_memory:.1f} MB")
    print(f"Memory difference: {final_memory - initial_memory:.1f} MB")

    # Verify first and last bytes
    print(f"\nFirst 50 bytes of response:")
    print(response_data[:50])
    print(f"\nLast 50 bytes of response:")
    print([b for b in response_data[-50:]])

if __name__ == "__main__":
    test_chunked_transfer()