#!/bin/sh

set -e
set -x

if [ "$2" != "" ]
then
    lo=$2
else
    lo=0
fi

if [ $lo -eq 0 ]
then
    killall lldb || true
    sleep 1
    killall constellation || true
    sleep 1
    killall lldb || true
    clear
    (cd build/;make -j100 constellation)
    ( rm -v data-*/* || true)
    for i in `ls build/swarmlog/*`
    do
        echo "" > $i
    done
fi
./apps/demoswarm/runswarm.py --binary ./build/apps/constellation/constellation --nodetype ConstellationNode --members $1 --lo $lo
