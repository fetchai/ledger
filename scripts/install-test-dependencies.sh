#!/bin/bash
set -e

pip3 install --no-cache-dir --ignore-installed --force-reinstall wheel pyyaml requests
pip3 install --no-cache-dir --ignore-installed --force-reinstall --editable git+https://github.com/fetchai/ledger-api-py.git@master#egg=fetchai-ledger-api
pip3 install --no-cache-dir --ignore-installed --force-reinstall ./scripts/fetchai_netutils
