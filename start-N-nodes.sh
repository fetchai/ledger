#!/bin/sh

set -e
set -x

killall lldb || true
sleep 1
killall constellation || true
sleep 1
killall lldb || true
clear
(cd build/;make -j100 constellation)
( rm -v data-*/* build/swarmlog/* || true )
./apps/demoswarm/runswarm.py --binary ./build/apps/constellation/constellation --nodetype ConstellationNode --members $1
