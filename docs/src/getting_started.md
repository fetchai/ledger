#<cldoc:Developers Guide::Getting Started>

The initial steps to get started.


# Installing dependencies
The Fetch ledger only depends on CMake, OpenSSL and ASIO. Optionally, it
depends on the Python development libraries if you want to build the
Python bindings. There currently is an additional dependency which will
be removed over time, namely libpng. This dependency only needs to be
installed on Ubuntu, but not OS X. ASIO is checked out as a submodule
and hence, you do not need to install anything. For OpenSSL and CMake do
following on Mac: 

```
  sudo port install cmake openssl
```

On Ubuntu / Debian:

```
  sudo apt-get install cmake libssl-dev libpng-dev python-dev
```  

# Download
The Fetch ledger is kept in the repository [[https://github.com/uvue-git/fetch-ledger]]. First thing to do, is to checkout the repository:

```
  cd [working_directory]
  git clone git@github.com:uvue-git/fetch-ledger.git
```

Next initialise submodules:

```
  cd fetch-ledger
  git submodule init
  git submodule update --recursive --remote
```

# Building
Assuming that you are in the Fetch ledger repository, you need to do following

```
  mkdir build
  cd build
  cmake ..
  make
```

in order to build the library. If you use Brew as your package manager on OS X, you will need to run 

```
  cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl .
```

You can configure the build by running

```
  ccmake .
```

right before make.

