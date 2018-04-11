Initiate
========
git submodule update --recursive --remote


Build
=====

mkdir build
cd build
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
cmake -DPYTHON_LIBRARY=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 .
cmake -DPYTHON_LIBRARY=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 -DPYTHON_EXECUTABLE=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/bin/python2.7 ..
