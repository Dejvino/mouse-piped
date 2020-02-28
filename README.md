# Mouse Piped
Send keyboard and mouse events remotely to another computer through a pipe. Control different devices using just one set of keyboard and mouse.

# Use Cases
* Multiple desktops on a table controlled as one
* Mobile device controlled from a PC

# Description
Low-level forwarding of input events (keyboard / mouse / touchpad / etc.) from one device to another. *Producer* system transmits an event stream from real mouse and keyboard events. *Consumer* system receives the stream and replicates the events using a fake input device.

# Requirements
* Linux on all devices. Successfully tested across different architectures: AMD64 and ARMv8.
* `/dev/input/*` for reading input device events.
* `/dev/uinput` for creating a virtual input device.

# Installation
Do the following on both devices:
1. `git clone <repository URL>`
2. `make`

# Running
## Local Testing
Producer generates a text stream of events:
```
./producer -v /dev/input/event*
```

You can pipe this directly into a consumer. To prevent it from messing with your keyboard and mouse you can run it in dummy mode:
```
./producer /dev/input/event* | ./consumer -v -d
```

### Note on permissions
* The *producer* process needs permissions to `/dev/input/event*` devices.
* The *consumer* process needs permissions to `/dev/uinput` device.
You can either run both as root or add your user to a group `input` and make sure the files belong to this group with read+write access.

## Pipe over SSH
Now things get interesting. You can easily pipe the stream over SSH!
```
./producer /dev/input/event* | ssh 10.0.0.2 -t "~/mouse-piped/consumer"
```

# Next Steps
* Add `-e` parameter when executing the producer to make it run endlessly. Otherwise there is a safety timeout that shuts down the process automatically. This is to prevent any input grab you couldn't escape during testing.
* Build some UI above this to automate the producer-consumer link creation.
