#!/bin/sh

. ./docker-env.sh

#BASEDIR=$(dirname "$0")
#echo "$BASEDIR"
echo "$LABEL"

#docker run -u root:root -it --rm -v "$(pwd):/build" $LABEL $@
docker run -it --rm -v "$(pwd):/build" $LABEL $@

