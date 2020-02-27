#!/bin/bash

CLIENT=10.0.0.2

stdbuf -o0 ./server | stdbuf -o0 ssh $CLIENT -x "cat > ~/mouse-piped/events"

