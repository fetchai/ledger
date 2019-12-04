#!/bin/bash
set -e

git config --global core.pager cat
git log --oneline -n 10

pip3 install --no-cache-dir --ignore-installed --force-reinstall wheel pyyaml requests
pip3 install --no-cache-dir --ignore-installed --force-reinstall fetchai-ledger-api==0.10.0a5
pip3 install --no-cache-dir --ignore-installed --force-reinstall ./scripts/fetchai_netutils
