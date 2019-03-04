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

#include "ledger/chain/transaction.hpp"
#include "ledger/executor_interface.hpp"

#include <atomic>
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
  using TxDigest  = Transaction::TxDigest;
  using LaneSet   = std::unordered_set<LaneIndex>;
  using Status    = ExecutorInterface::Status;

  static constexpr char const *LOGGING_NAME = "ExecutionItem";

  ExecutionItem(TxDigest hash, std::size_t slice)
    : hash_(std::move(hash))
    , slice_(slice)
  {}

  ExecutionItem(TxDigest hash, LaneIndex lane, std::size_t slice)
    : hash_(std::move(hash))
    , lanes_{lane}
    , slice_(slice)
  {}

  TxDigest hash() const
  {
    return hash_;
  }

  LaneSet const &lanes() const
  {
    return lanes_;
  }

  std::size_t slice() const
  {
    return slice_;
  }

  Status status() const
  {
    return status_;
  }

  void Execute(ExecutorInterface &executor)
  {
    try
    {
      status_ = executor.Execute(hash_, slice_, lanes_);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Exception thrown be executing transaction: ", ex.what());

      status_ = Status::RESOURCE_FAILURE;
    }
  }

  void AddLane(LaneIndex lane)
  {
    lanes_.insert(lane);
  }

private:
  using AtomicStatus = std::atomic<Status>;

  TxDigest     hash_;
  LaneSet      lanes_;
  std::size_t  slice_;
  AtomicStatus status_{Status::NOT_RUN};
};

}  // namespace ledger
}  // namespace fetch
