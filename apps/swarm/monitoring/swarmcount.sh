#!/bin/sh
while true
do
    ps -ef | grep apps/pyfetch/pyfetch | grep -- "-id" | grep -v bin/sh | grep -v python | grep -v grep | wc -l
    sleep 1
done

