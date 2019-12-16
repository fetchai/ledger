#!/bin/bash
set -e

pip3 install wheel --force-reinstall
pip3 install pyyaml requests --force-reinstall
pip3 install fetchai-ledger-api==0.11.0a4 --force-reinstall
pip3 install ./scripts/fetchai_netutils --force-reinstall
