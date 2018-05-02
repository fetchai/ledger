Configure git
=============
Depending on transport (`ssh` or `https`) you prefere to access github, configure your local git settings based on [this guide](https_ssh_git_config.md).

Initiate
========
git submodule update --recursive --remote


Build
=====

mkdir build
cd build
cmake ..
make


On Ubuntu
=========

sudo apt-get install libssl-dev cmake
sudo apt-get install libpng-dev
sudo apt-get install python-dev

Test
====

ctest


Docker - build, develop & run stuff in docker container
=======================================================
This project has full support for Docker - it can be used to build whole project, execute resulting binaries and do whole development using docker, please see details in [this guide](README-docker.md)

### Using docker for development purposes in this repository (build code & run stuff)

> Pulling image to your local docker cache
```bash
develop-image/scripts/docker-pull-img.sh
```

> Start `bash` shell in the docker container. This allows to run further command (e.g. cmake, make, etc. ...) there: 
```bash
develop-image/scripts/docker-run.sh bash
```

> Executing `make` using convenience script - it will `cmake` and then `make` in `build` folder in docker container, and then exits the docker container. All options passed to this script are passed to the `make`:
```bash
develop-image/scripts/docker-make.sh all
```

> Building the image locally. It is sefull when tinkering with setup of the docker image in `develop-image/` folder (e.g. tinkering with `develop-image/Dockerfile`, `develop-image/docker-env.sh`, etc. ...): 
```bash
develop-image/scripts/docker-build-img.sh
# Or use the `--no-cached` option to make sure image will get completelly
# rebuild even if it exists in local docker cache.
# (NOTE: the ending `--` is necessary to be provided)
#develop-image/scripts/docker-build-img.sh --no-cached --
```

> Publising (pushing) image in to docker registry (our google clound project docker registry):
```bash
develop-image/scripts/docker-publis-img.sh
```

Notes
=====
If network gives random read/write errors, it can be an indication that
not enough threads are available. Long term we need to have a logging
mechanism for when this happens to ensure we can meet demand. 
cmake -DPYTHON_LIBRARY=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 .
cmake -DPYTHON_LIBRARY=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 -DPYTHON_EXECUTABLE=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/bin/python2.7 ..
