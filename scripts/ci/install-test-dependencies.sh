#!/bin/bash
set -e

# install the latest version of the ledger api
pip3 install git+https://github.com/fetchai/ledger-api-py.git@develop

# install the latest version of the net utils
pip3 install -i https://test.pypi.org/simple/ fetchai-netutils==0.0.1
