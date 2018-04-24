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

Notes
=====
If network gives random read/write errors, it can be an indication that
not enough threads are available. Long term we need to have a logging
mechanism for when this happens to ensure we can meet demand. 
cmake -DPYTHON_LIBRARY=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 .
cmake -DPYTHON_LIBRARY=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 -DPYTHON_EXECUTABLE=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/bin/python2.7 ..
