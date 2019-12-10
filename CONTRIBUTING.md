# Contributing to Fetch.ai ledger

:+1::tada: First off, thanks for taking the time to contribute! :tada::+1:

The following is a set of guidelines for contributing to the [Fetch.ai ledger repository](https://github.com/fetchai/ledger) on GitHub. We strongly encourage you to follow them as closely as possible; if you are dealing with a case not specified here, use your best judgment and feel free to propose changes to the document in a pull request.


## Bug reports

Before creating a bug report, please check [this list](https://github.com/fetchai/ledger/issues) as you might find out that you don't need to create one. When you are creating a bug report, please include as many details as possible.

**Note:** If you find a **Closed** issue that seems like what you're experiencing, open a new issue and include a link to the original issue in the body of your new one.


## Code contributions

### Style guidelines

When developing new code or modifying existing one, please follow our [C++ style guidelines](docs/cplusplus-style-guide.md). They describe file structuring, resource handling, etc. Some basic instructions about formatting are also included, but please refer to the [ClangFormat file](.clang-format) for the most detailed specification.

Adherence to these guidelines will be automatically checked by the CI system, but in order to speed up the build process, we recommend you run these checks locally during development with the command `./scripts/apply_style.py -d`.

### Development workflow

We favour forking instead of creating multiple branches in the upstream repository, where `master` is the default branch. For more details about branching and merging, please read our [branching model documentation](docs/branching-model.md).

If you are going to tag a version in preparation for release, please read our [release strategy](docs/release-strategy.md) draft.

### Copyright and licensing header

Please include the following header at the beginning of every source file, replacing `<current year>` accordingly.

```c++
//------------------------------------------------------------------------------
//
//   Copyright 2018-<current year> Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------
```
