Configure git
=============
Depending on transport (`ssh` or `https`) you prefere to access github, configure your local git settings based on [this guide](https://github.com/uvue-git/docker-images/blob/master/README_git_setup.md).

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

NOTE: the whole necessary setup is defined by the [Dockerfile](https://github.com/uvue-git/docker-images/blob/master/fetch-ledger-develop-image/Dockerfile), also checkout the [Docker](#docker) section bellow for more details about our docker setup.


Test
====

ctest

Docker - build, develop & run stuff in docker container<a name="docker"></a>
=======================================================
This project has full support for Docker - it is dedicated to be used for building whole project, execute resulting binaries and do whole development using docker, please see details in [this guide](uvue-git/docker-images/README.md#guick_usage_guide).

Notes
=====
If network gives random read/write errors, it can be an indication that
not enough threads are available. Long term we need to have a logging
mechanism for when this happens to ensure we can meet demand. 
cmake -DPYTHON_LIBRARY=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 .
cmake -DPYTHON_LIBRARY=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 -DPYTHON_EXECUTABLE=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/bin/python2.7 ..
