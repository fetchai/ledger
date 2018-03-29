Initiate
========
git init submodule
git submodule update --recursive --remote

This might not install asio in ./vendor/asio/asio . If it has not, install asio:
cd ./vendor
git clone https://github.com/chriskohlhoff/asio

Mac:
sudo port install cmake openssl

On Ubuntu / Debian:
sudo apt-get install cmake libssl-dev libpng-dev python-dev

Build
=====

mkdir build
cd build
cmake ..
make

If you use Brew as your package manager on OS X, you will need to run
cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl .

Test
====

ctest


Notes
=====
If network gives random read/write errors, it can be an indication that
not enough threads are available. Long term we need to have a logging
mechanism for when this happens to ensure we can meet demand. 
