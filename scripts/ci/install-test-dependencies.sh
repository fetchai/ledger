#!/bin/bash
set -e

pip3 install wheel

#???
pip3 install -e git+https://github.com/fetchai/ledger-api-py.git@wip_consolidate_smart_and_synergetic_contracts#egg=fetchai-ledger-api

pip3 install fetchai-ledger-api==0.5.1
pip3 install -i https://test.pypi.org/simple/ fetchai-netutils==0.0.6a1

pip3 install pyyaml
