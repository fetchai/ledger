#!/bin/sh
while true
do
    ps -ef | grep build/examples/swarm | grep -- "-id" | grep -v bin/sh | grep -v python | grep -v grep | wc -l
    sleep 1
done

