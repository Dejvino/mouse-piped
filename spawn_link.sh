#!/bin/bash

CONSUMER=10.0.0.2
DEVICES=/dev/input/event*

./producer -e $DEVICES | ssh $CONSUMER -t "~/mouse-piped/consumer"
