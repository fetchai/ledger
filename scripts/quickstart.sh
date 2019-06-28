#!/bin/bash

onred='\033[41m'
ongreen='\033[42m'
onyellow='\033[43m'
endcolor="\033[0m"

# Handle errors
set -e
error_report() {
    echo -e "${onred}Error: failed on line $1.$endcolor"
}
trap 'error_report $LINENO' ERR

echo -e "${onyellow}Installing Fetch Ledger...$endcolor"

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    sudo apt-get update
    sudo apt-get install -y libssl-dev cmake python3-dev clang git
elif [[ "$OSTYPE" == "darwin"* ]]; then
    xcode-select --version || xcode-select --install
    brew --version || yes | /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    python3.7 --version || brew install python
    cmake --version || brew install cmake
    brew install openssl
    for version in `ls -t /usr/local/Cellar/openssl/`; do
        ossl=/usr/local/Cellar/openssl/$version
        break
    done
    if [ -z "$ossl" ]; then
        echo -e "${onred}Error: OpenSSL not installed.$endcolor"
        exit 1
    fi
fi
# Checks if run from the scripts directory.
if [ ${PWD##*/} == scripts ]; then
    cd ..
fi
git submodule update --init --recursive
if [ -d build ]; then
    rm -rf build
fi
mkdir build
cd build
if [[ "$OSTYPE" == "darwin"* ]]; then
    cmake ../ -DOPENSSL_ROOT_DIR=$ossl
else
    cmake ../
fi
make -j 4

echo -e "${ongreen}Fetch Ledger installed.$endcolor"
