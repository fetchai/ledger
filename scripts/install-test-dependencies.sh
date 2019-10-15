#!/bin/bash
set -e

pip3 install --no-cache-dir --ignore-installed --force-reinstall wheel pyyaml requests
# ???
pip3 install  --no-cache-dir --ignore-installed --force-reinstall --editable git+https://github.com/wkaluza/ledger-api-py.git@pr_nonce_contract_address#egg=fetchai-ledger-api
# pip3 install --no-cache-dir --ignore-installed --force-reinstall fetchai-ledger-api==0.9.0a3
pip3 install --no-cache-dir --ignore-installed --force-reinstall ./scripts/fetchai_netutils
