#!/bin/sh

. ./docker-env.sh

docker build --rm -t $LABEL .

