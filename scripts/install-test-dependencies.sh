#!/bin/bash
set -e

pip3 install wheel --force-reinstall
pip3 install pyyaml requests --force-reinstall
pip3 install fetchai-ledger-api==1.0.0rc1 --force-reinstall
pip3 install ./scripts/fetchai_netutils --force-reinstall
