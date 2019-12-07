#!/bin/bash
set -e

pip3 install  wheel pyyaml requests --force-reinstall
pip3 install  fetchai-ledger-api==0.10.2 --force-reinstall
pip3 install ./scripts/fetchai_netutils --force-reinstall
