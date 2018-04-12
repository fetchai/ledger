#!/bin/sh

. ./docker-env.sh

#docker run -u root:root -p 8080:8080 -it --rm -v "$(pwd):/build" $LABEL $@
docker run -p 8080:8080 -it --rm -v "$(pwd):/build" $LABEL $@

