# Ledger

Welcome to Fetch.AI ledger repository. We are building the digital world for today, and the future.

## License

Fetch.AI Ledger is licensed under the Apache software license (see LICENSE file). Unless required by
applicable law or agreed to in writing, software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either \express or implied.

Fetch.AI makes no representation or guarantee that this software (including any third-party libraries)
will perform as intended or will be free of errors, bugs or faulty code. The software may fail which
could completely or partially limit functionality or compromise computer systems. If you use or
implement the ledger, you do so at your own risk. In no event will Fetch.AI be liable to any party
for any damages whatsoever, even if it had been advised of the possibility of damage.

As such this codebase should be treated as experimental and does not contain all currently developed
features. Fetch.AI will be delivering regular updates.

## Resources

1. [Website](https://fetch.ai/)
2. [Blog](https://fetch.ai/blog)
3. [Community Website](https://community.fetch.ai/)
4. [Community Telegram Group](https://t.me/fetch_ai)
5. [Whitepapers](https://fetch.ai/press-partners-publications/#publications)
6. [Roadmap](https://fetch.ai/fetch-ais-2019-technical-roadmap/)


## Supported platforms

* MacOS Darwin 10.13x and higher (64bit)
* Ubuntu 18.04 (x86_64)

(We plan to support all major platforms in the future).

## Getting Started

```
git clone https://github.com/fetchai/ledger.git
cd ledger
./scripts/quickstart.sh
```

Or follow our online documentation at [building the ledger](http://docs.fetch.ai/getting-started/installation-mac/).

## Connecting to a test network

Navigate to the constellation application folder:

```
cd build/apps/constellation
```

Optionally delete the database files (in the case where you have been running a local network):

```
rm -f *.db
```

Connect to the `alpha` test network:

```
git checkout release/v0.7.x
./scripts/quickstart.sh
./constellation -bootstrap -network alpha
```

## Running the ledger locally

Alternatively, you can run the ledger locally (1 second block interval in this case):

```
./constellation -standalone -block-interval 1000
```
