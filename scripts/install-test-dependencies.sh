#!/bin/bash
set -e

pip3 install wheel --force-reinstall
pip3 install pyyaml requests --force-reinstall
pip3 install lark-parser==0.7.8 --force-reinstall
pip3 install fetchai-ledger-api==1.0.0rc1 --force-reinstall
pip3 install ./scripts/fetchai_netutils --force-reinstall

# display the list of dependencies that can be inspected later
pip3 freeze
