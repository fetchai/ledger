#!/bin/bash

cd ${0%/*}
. ./docker-env.sh

docker build --build-arg http_proxy=$http_proxy --build-arg https_proxy=$https_proxy --build-arg no_proxy=$no_proxy -t $DOCKER_IMAGE_TAG --no-cache $@ .

