#!/bin/sh

build/apps/pyfetch/pyfetch apps/pyfetch/pychainnode.py -id 12 -maxpeers 3 -port 9012 -peerlist 127.0.0.1:9000,127.0.0.1:9001 -solvespeed 1000 -idlespeed 100
