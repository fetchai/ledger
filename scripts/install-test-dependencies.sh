#!/bin/bash
set -e

pip3 install  wheel pyyaml requests --force-reinstall
pip3 install --no-cache-dir --ignore-installed --force-reinstall --editable git+https://github.com/wkaluza/ledger-api-py.git@pr_parser_c2c_fix#egg=fetchai-ledger-api
# ??? Restore and bump version when changes merged into SDK
# pip3 install  fetchai-ledger-api==0.10.2 --force-reinstall
pip3 install ./scripts/fetchai_netutils --force-reinstall
