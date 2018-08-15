#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
  using counter_type = std::atomic<std::size_t>;

  DummyContract();
  ~DummyContract() = default;

  std::size_t counter() const { return counter_; }

private:
  Status Wait(transaction_type const &tx);
  Status Run(transaction_type const &tx);

  counter_type counter_{0};
};

}  // namespace ledger
}  // namespace fetch
