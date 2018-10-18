#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include <utility>

namespace fetch {
namespace ledger {

class ExecutionItem
{
public:
  using LaneIndex = uint32_t;
  using TxDigest  = chain::Transaction::TxDigest;
  using LaneSet   = std::unordered_set<LaneIndex>;

  ExecutionItem(TxDigest hash, std::size_t slice)
    : hash_(std::move(hash))
    , slice_(slice)
  {}

  ExecutionItem(TxDigest hash, LaneIndex lane, std::size_t slice)
    : hash_(std::move(hash))
    , lanes_{lane}
    , slice_(slice)
  {}

  ExecutorInterface::Status Execute(ExecutorInterface &executor)
  {
    return executor.Execute(hash_, slice_, lanes_);
  }

  void AddLane(LaneIndex lane)
  {
    lanes_.insert(lane);
  }

private:
  TxDigest    hash_;
  LaneSet     lanes_;
  std::size_t slice_;
};

}  // namespace ledger
}  // namespace fetch
