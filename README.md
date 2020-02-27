# Mouse Piped
Send keyboard and mouse events remotely to another computer through a pipe. Control different devices using just one set of keyboard and mouse.

# Installation
1. `git clone` this repo into ~/mouse-piped on both server and client.
2. `make client` and `make server` on respective devices.

# Running
## Client
```
~/mouse-piped/spawn_client.sh

# or without the script:
cd ~/mouse-piped/
mkfifo events
tail -f events | ./client
```

## Server
```
CLIENT=10.0.0.2
./server | ssh $CLIENT -x "cat > ~/mouse-piped/events"
```

