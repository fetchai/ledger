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

#include "ledger/chain/transaction.hpp"
#include "ledger/executor_interface.hpp"

#include <cstdint>
#include <future>
#include <memory>

namespace fetch {
namespace ledger {

class ExecutionItem
{
public:
  using lane_index_type = uint32_t;
  using tx_digest_type  = chain::Transaction::digest_type;
  using lane_set_type   = std::unordered_set<lane_index_type>;

  ExecutionItem(tx_digest_type const &hash, std::size_t slice) : hash_(hash), slice_(slice) {}

  ExecutionItem(tx_digest_type const &hash, lane_index_type lane, std::size_t slice)
    : hash_(hash), lanes_{lane}, slice_(slice)
  {}

  ExecutorInterface::Status Execute(ExecutorInterface &executor)
  {
    return executor.Execute(hash_, slice_, lanes_);
  }

  void AddLane(lane_index_type lane) { lanes_.insert(lane); }

private:
  tx_digest_type hash_;
  lane_set_type  lanes_;
  std::size_t    slice_;
};

}  // namespace ledger
}  // namespace fetch
