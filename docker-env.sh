#!/bin/bash

DOCKER_IMAGE_TAG=fetch-ledger-build:latest
WORKDIR=/build

echo http_proxy =$http_proxy
echo https_proxy=$https_proxy
echo no_proxy=$no_proxy

# Description: This function splits commandline parameters to two groups - left
# and right groups of parameters. 
# The split to this two groups is controlled by the '--' commandline parameter
# separator, where all parameters provided to the left of this separator will
# located in the left parameters group, and all parameters to the right of this
# separator will be located in the right parameters group.
# This separator is not mandatory, so if it is not provided, then all provided
# commandline parameters will be placed in to be in the right parameters group.
# If this separator is provided, then the format is this: p1 p2 ... pN -- pN+2 pN+3 ... pN+M
# , where parameetrs p1, p2, ..., pN will be assigned to te left parametrs group,
# and pN+2, pN+3, ..., pN+M will be assigned to the right parameters group.
# Also, any and all of `p1, p2, ..., pN`, or `--`, or  `pN+2, pN+3, ..., pN+M` do *not* need to be provided, and so can be omitted. If all of them are omitted, then the `--` special parameter can be omitted as well.
# Usage of this shell function:
#   # The function passes the results (left and right params groups) as two input 
#   # parameters to the callback shell function where caller can implement necessary
#   # business logic:
#   callback() {
#     local LEFT_PARAMS="$1"
#     local RIGHT_PARAMS="$2"
#     echo LEFT_PARAMS="$LEFT_PARAMS"
#     echo RIGHT_PARAMS="$RIGHT_PARAMS"
#   }
#   # ESSENTIAL: The "$@" must be provided as second argument WITH QUOTATIONS AROUND IT
#   split_params callback "$@"
#
# Usage in docker shell scripts: 
#   ./docker-run.sh <docker parameters> -- <executable> <executable parameters>
#   ./docker-make.sh <docker parameters> -- <make parameters>
# Examples:
#   # The following example provides the `--cpus 4` parameter to `docker run` command (sets number of available CPUs for docker run) and executes `make -j http_server` executable with parameters inside of the docker container
#   ./docker-run.sh --cpus 4 -- make -j http_server
#
#   # The following example provides `-p 8080:8080` parameter to `docker run` command (exports inner docker network port 8080 to outer(host OS) port 8080) and excutes `http_server` executable inside of the docker container: 
#   ./docker-run.sh -p 8080:8080 -- build/examples/http_server

split_params()
{
  local CALLBACK_FNC_NAME=$1
  local LEFT_PARAMS_GROUP=
  local RIGHT_PARAMS_GROUP=
  local FOUND=false
  local whitespace="[[:space:]]"

  for i in "${@:2:$#-1}"; do
    if ( [ "$FOUND" != "true" ] && [ "$i" == "--" ] ); then
      FOUND=true
      LEFT_PARAMS_GROUP=$RIGHT_PARAMS_GROUP
      RIGHT_PARAMS_GROUP=
      continue;
    fi

    local escaped_i=$i
    if [[ $i =~ $whitespace ]]
    then
      escaped_i=\"${i/\"/\\\"}\"
    fi

    RIGHT_PARAMS_GROUP=$RIGHT_PARAMS_GROUP" "$escaped_i
  done

  "$CALLBACK_FNC_NAME" "$LEFT_PARAMS_GROUP" "$RIGHT_PARAMS_GROUP"
}

