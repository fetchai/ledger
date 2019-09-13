#!/bin/bash
set -e

pip3 install --no-cache-dir --ignore-installed --force-reinstall wheel pyyaml requests
# ???
pip3 install --no-cache-dir --ignore-installed --force-reinstall git+https://github.com/wkaluza/ledger-api-py.git@wip_consolidate_smart_and_synergetic_contracts#egg=fetchai-ledger-api
pip3 install --no-cache-dir --ignore-installed --force-reinstall --index-url https://test.pypi.org/simple/ fetchai-netutils==0.0.6a1
