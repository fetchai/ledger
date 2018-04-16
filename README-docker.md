Docker
======

There has been made docker setup in this project which allows to fully build, develop and run the built binaries.

This allows absolutly clear and reproducible setup (build enviromnent, test environment and development environment) no matter what host OS is used to actual hosting and development.

The setup is done the way that it supports comfortable development - you can edit files locally on your host OS, and just build & run binaries in docker. Build & run is simplifiead to bare minimum so you won't feel any overhead in terms of number of commands/steps you need to perform, or performance, comparing to building & runing stuff directly on local host OS.

## Filesystem sharing
The `docker-run.sh` & `docker-make.sh` scripts are made the way that your local host OS filesystem folder where you have cloned your `fetch-ledger` repository will be **shared** with running docker container where it will be mapped in to `/build` folder.
This means that you can edit you files locally on you host OS using editors/tools you like, and the changes will be visible inside of already running or newly started docker container (and vice-versa). 

Installation & setup
====================

## Installation
Download the Docker (Community Edition) installtion package for your plafrom from https://store.docker.com/search?type=edition&offering=community (or alternativelly use more general https://www.docker.com/community-edition instead if the previous link does not work for you) and install it.

## Setup
Find Docker icon in system status bar (on Mac OS X top right side of the screen, or bottom right on Windows OS). Rightclick on the the `Preferences`. There go to the `Advanced` tab where pull all resources to the maximum (CPU, Memory, Swap, etc. ...), don't worry Docker will use on demand (momentarily) only as much resources as it actually needs, what means it immediatelly releases resources which are not used.
If your network setup needs to use proxy, it can be done in the `Proxies` tab.
==> **Click at `Apply & Restart` to reflect changes**, Docker daemon will restartfully is 
, wait until the daemon is fully up & running again.

Usage in our project
====================

## Build docker image
    This steps is just one off, necessary to be executed only once after `Dockerfile` has been changed.
    ```sh
    cd fetch-ledger
    ./docker-build-img.sh
    ```

## Execute any executable available in the docker image:
    ```sh
    ./docker-run.sh PATH_TO_EXECUTABLE
    ```

## Get `bash` shell running in the docker containe:
    ```sh
    ./docker-run.sh bash
    ```
    This shell can be used to execute anything you need, e.g. running `cmake`, `make`, etc. ...

## Run `make` directly from host OS without necessity to run `bash` in docker container:
    ```sh
    ./docker-make.sh [MAKE_OPTIONS] MAKE_TARGET
    ```

    , for example:
    ```sh
    ./docker-make.sh http_server
    ```

    The `./docker-make.sh` behavew as it would be directly `make` (all parameters provided are directly transferred to the `make` executable). In order to simplify the build process iit creates the `build` folder (**if** it does not exists), cd's in to it, runs `cmake ..` there, and only after that runs the `make` finally.

