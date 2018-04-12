#!/bin/sh

. ./docker-env.sh

#BASEDIR=$(dirname "$0")
#echo "$BASEDIR"
echo "$LABEL"

docker run -u 0:0 -it --rm -v "$(pwd):/build" $LABEL $@

