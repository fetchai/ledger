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

#include "ledger/chain/digest.hpp"
#include "ledger/executor_interface.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "storage/resource_mapper.hpp"

#include <chrono>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

class FakeExecutor : public fetch::ledger::ExecutorInterface
{
public:
  using Digest    = fetch::ledger::Digest;
  using Address   = fetch::ledger::Address;
  using BitVector = fetch::BitVector;
  using Clock     = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;

  struct HistoryElement
  {
    Digest     digest;
    BlockIndex block;
    SliceIndex slice;
    BitVector  shards;
    Timepoint  timestamp;
  };

  using HistoryElementCache = std::vector<HistoryElement>;
  using StorageInterface    = fetch::ledger::StorageInterface;

  Result Execute(Digest const &digest, BlockIndex block, SliceIndex slice,
                 BitVector const &shards) override
  {
    history_.emplace_back(HistoryElement{digest, block, slice, shards, Clock::now()});

    // if we have a state then make some changes to it
    if (state_)
    {
      state_->Set(fetch::storage::ResourceAddress{digest}, "executed");
    }

    return {Status::SUCCESS};
  }

  void SettleFees(Address const &miner, TokenAmount amount, uint32_t log2_num_lanes) override
  {
    FETCH_UNUSED(miner);
    FETCH_UNUSED(amount);
    FETCH_UNUSED(log2_num_lanes);
  }

  std::size_t GetNumExecutions() const
  {
    return history_.size();
  }

  void CollectHistory(HistoryElementCache &history)
  {
    history_.reserve(history.size() + history_.size());  // do the allocation
    history.insert(history.end(), history_.begin(), history_.end());
  }

  void SetStorageInterface(StorageInterface &state)
  {
    state_ = &state;
  }

  void ClearStorageInterface()
  {
    state_ = nullptr;
  }

private:
  StorageInterface *  state_ = nullptr;
  HistoryElementCache history_;
};
