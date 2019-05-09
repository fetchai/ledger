# Ledger

Welcome to Fetch ledger repository. We are building the digital world for today, and the future.

## License

Fetch Ledger is licensed under the Apache software license (see LICENSE file). Unless required by
applicable law or agreed to in writing, software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either \express or implied.

Fetch.AI makes no representation or guarantee that this software (including any third-party libraries)
will perform as intended or will be free of errors, bugs or faulty code. The software may fail which
could completely or partially limit functionality or compromise computer systems. If you use or
implement the ledger, you do so at your own risk. In no event will Fetch.AI be liable to any party
for any damages whatsoever, even if it had been advised of the possibility of damage.

As such this codebase should be treated as experimental and does not contain all currently developed
features. Fetch will be delivering regular updates.

## Resources

1. [Website](https://fetch.ai/)
2. [Blog](https://fetch.ai/blog)
3. [Community Website](https://community.fetch.ai/)
4. [Community Telegram Group](https://t.me/fetchai)
5. [Whitepapers](https://fetch.ai/publications.html)
6. [Roadmap](https://fetch.ai/#/roadmap)


## Supported platforms

* MacOS Darwin 10.13x and higher (64bit)
* Ubuntu 18.04 (x86_64)

(We plan to support all major platforms in the future)

## Getting Started

A more complete guide is available in our [Getting Started Guide](docs/source/getting_started/index.rst).
However, the following section outlines some of the initial steps.

To get started, ensure all the code along with the submodules has been checked out with the
following commands:

    git clone https://github.com/fetchai/ledger.git

    cd ledger

    git submodule update --init --recursive

## Dependencies

### Ubuntu

    sudo apt-get install libssl-dev cmake python3-dev clang

### MacOS

    sudo brew install cmake openssl

## Building the code

The project uses cmake so you can following formal build procedure of:

    mkdir build

    cd build

    cmake ../

    make -j

## Connecting to a test network

Connecting and joining the test net is relatively straight forward. This first thing to do is to
ensure that you have build the latest version of the `constellation` application.

    cd build

    cmake ../

    make -j constellation

Navigate to the constellation application folder:

    cd apps/constellation

Optionally delete the database files (in the case where you have been running a local network)

    rm -f *.db

Start the network connecting to the `alpha` test network.

    ./constellation -bootstrap -network alpha
