Initiate
========
git submodule update --recursive --remote


Build
=====

mkdir build
cmake ..
make


Test
====

ctest


Notes
=====
If network gives random read/write errors, it can be an indication that
not enough threads are available. Long term we need to have a logging
mechanism for when this happens to ensure we can meet demand. 
