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

#include "core/logger.hpp"
#include "ledger/executor_interface.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "storage/resource_mapper.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <utility>

class FakeExecutor : public fetch::ledger::ExecutorInterface
{
public:
  static constexpr char const *LOGGING_NAME = "FakeExecutor";

  struct HistoryElement
  {
    using clock_type     = std::chrono::high_resolution_clock;
    using timepoint_type = clock_type::time_point;

    HistoryElement(TxDigest const &h, std::size_t s, LaneSet l)
      : hash(h)
      , slice(s)
      , lanes(std::move(l))
    {}
    HistoryElement(HistoryElement const &) = default;

    TxDigest       hash;
    std::size_t    slice;
    LaneSet        lanes;
    timepoint_type timestamp{clock_type::now()};
  };

  using HistoryElementCache = std::vector<HistoryElement>;
  using StorageInterface    = fetch::ledger::StorageInterface;

  Status Execute(TxDigest const &hash, std::size_t slice, LaneSet const &lanes) override
  {
    history_.emplace_back(hash, slice, lanes);

    // if we have a state then make some changes to it
    if (state_)
    {
      state_->Set(fetch::storage::ResourceAddress{hash}, "executed");
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Executing transaction sort of...");

    return Status::SUCCESS;
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
