#!/bin/bash
set -e

pip3 install  wheel pyyaml requests --force-reinstall
pip3 install  fetchai-ledger-api==v0.11.0a3 --force-reinstall
pip3 install ./scripts/fetchai_netutils --force-reinstall
