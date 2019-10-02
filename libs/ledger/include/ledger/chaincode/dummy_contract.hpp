#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "ledger/chaincode/contract.hpp"

#include <atomic>

namespace fetch {
namespace ledger {

class DummyContract : public Contract
{
public:
  static constexpr char const *NAME = "fetch.dummy";

  DummyContract();
  ~DummyContract() override = default;

  static constexpr char const *LOGGING_NAME = "DummyContract";

  std::size_t counter() const
  {
    return counter_;
  }

private:
  using Counter = std::atomic<std::size_t>;

  Result Wait(Transaction const &tx, BlockIndex /*index*/);
  Result Run(Transaction const &tx, BlockIndex /*index*/);

  Counter counter_{0};
};

}  // namespace ledger
}  // namespace fetch
