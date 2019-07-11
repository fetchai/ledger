#!/bin/bash
set -e

pip3 install wheel

pip3 install fetchai-ledger-api==0.5.1
pip3 install -i https://test.pypi.org/simple/ fetchai-netutils==0.0.5a1

pip3 install pyyaml
