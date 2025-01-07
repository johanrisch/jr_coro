#!/bin/bash

rm file1 file2 file3 > /dev/null 2>&1
set -oue pipefail

# Start the jr_tcp_server in the background
./jr_tcp_server > /dev/null 2>&1 & 
SERVER_PID=$!

(
    echo "Hello, World!"
    sleep 0.1
) | telnet localhost 8080 > file1 2>&1 &

(
    echo "Hello, World!"
    sleep 0.1
) | telnet localhost 8080 > file2 2>&1 &

(
    echo "Hello, World!"
    sleep 0.1
) | telnet localhost 8080 > file3 2>&1

kill -9 $SERVER_PID > /dev/null 2>&1

cat file1 file2 file3 | grep "Hello, World!" > /dev/null && echo "OK!" || echo "FAIL!"
