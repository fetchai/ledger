#!/bin/bash
set -e

pip3 install --no-cache-dir --ignore-installed --force-reinstall wheel pyyaml requests
#pip3 install --no-cache-dir --ignore-installed --force-reinstall fetchai-ledger-api==0.10.0a3
pip3 install --no-cache-dir --ignore-installed --force-reinstall git+https://github.com/fetchai/ledger-api-py.git@a17e3160673636494f12eb60e115b6ca520de44d#egg=fetchai-ledger-api
pip3 install --no-cache-dir --ignore-installed --force-reinstall ./scripts/fetchai_netutils
