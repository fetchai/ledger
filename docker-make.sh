#/bin/bash
# Usage:
#   ./docker-make.sh <docker parameters> -- <parameters for make>
# Examples:
#   # The following example provides the `--cpus 4` parameter to `docker run` command (sets number of available CPUs for docker run) and executes make with `-j http_server` parameters inside of the docker container
#   ./docker-make.sh --cpus 4 -- http_server
#
# NOTE: For more details, please see description for the `split_params()` shell function in the `docker-env.sh` script.
cd ${0%/*}
. ./docker-env.sh

docker_run_callback() {
    local DOCKER_PARAMS="$1"
    local MAKE_PARAMS="$2"
    local COMMAND="./docker-run.sh $DOCKER_PARAMS -- ./docker-local-make.sh $MAKE_PARAMS"
    echo $COMMAND
    $COMMAND
}

split_params docker_run_callback "$@"

