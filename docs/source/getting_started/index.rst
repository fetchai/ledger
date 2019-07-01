Getting started
===============

Installing dependencies
-----------------------

The Fetch ledger only depends on CMake, OpenSSL and ASIO. Optionally, it depends on the Python development libraries if you want to build the Python bindings. There currently is an additional dependency which will be removed over time, namely libpng. This dependency only needs to be installed on Ubuntu, but not OS X. ASIO is checked out as a submodule and hence, you do not need to install anything. For OpenSSL and CMake do following on Mac:

.. code:: bash

	$ sudo brew install cmake openssl

Alternativly if you are using MacPorts:

.. code:: bash

	$ sudo port install cmake openssl

On Ubuntu / Debian:

.. code:: bash

	$ sudo apt install cmake cmake-curses-gui libssl-dev python3-dev clang-tidy clang-format

Download
--------

The Fetch ledger is kept in the repository https://github.com/fetchai/ledger.git. First thing to do, is to checkout the repository:

.. code:: bash

	$ cd [working_directory]
	$ git clone https://github.com/fetchai/ledger.git

Next initialise submodules:

.. code:: bash

	$ cd fetch-ledger
	$ git submodule update --init

Building
--------

Assuming that you are in the Fetch ledger repository, you need to do following to build the library:

.. code:: bash

	$ mkdir build
	$ cd build
	$ cmake ..
	$ make -j

If you use Brew as your package manager on OS X, before building the code you will need to define the location for cmake to find the openssl libraries. An example is shown below. It is recommened that you add this to `~/.bash_profiles` or similar configuration file.

.. code:: bash

	$ export OPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2o_2


Users can interactively configure the build by executing the following command inside the build directory to the project:

.. code:: bash

	$ ccmake .
