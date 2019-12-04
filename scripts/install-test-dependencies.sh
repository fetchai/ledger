#!/bin/bash
set -e

pip3 uninstall -y fetchai-ledger-api

pip3 install --no-cache-dir --ignore-installed --force-reinstall wheel pyyaml requests
pip3 install --no-cache-dir --ignore-installed --force-reinstall fetchai-ledger-api==0.10.2
pip3 install --no-cache-dir --ignore-installed --force-reinstall ./scripts/fetchai_netutils
