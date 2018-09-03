#!/bin/sh

set -e
set -x

debugfirst=$1
members=$1
if [ "$2" != "" ]
then
    members=$2
fi

./apps/demoswarm/runswarm.py --binary ./build/apps/constellation/constellation --nodetype ConstellationNode --members $members --debugger lldb --debugfirst $debugfirst
