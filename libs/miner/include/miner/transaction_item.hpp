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

#include <utility>

#include "ledger/chain/mutable_transaction.hpp"

namespace fetch {
namespace miner {

class TransactionItem
{
public:
  // Construction / Destruction
  TransactionItem(ledger::TransactionSummary tx, std::size_t id)
    : summary_(std::move(tx))
    , id_(id)
  {}
  ~TransactionItem() = default;

  ledger::TransactionSummary const &summary() const
  {
    return summary_;
  }

  std::size_t id() const
  {
    return id_;
  }

  // debug only
  std::unordered_set<std::size_t> lanes;

private:
  ledger::TransactionSummary summary_;
  std::size_t                id_;
};

}  // namespace miner
}  // namespace fetch
