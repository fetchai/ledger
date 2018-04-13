#!/bin/bash

cd ${0%/*}
. ./docker-env.sh
docker run -it --rm -v $(pwd):$WORKDIR $DOCKER_IMAGE_TAG $@

