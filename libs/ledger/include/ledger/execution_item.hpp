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

  ExecutionItem(TxDigest hash, uint32_t log2_num_lanes, LaneSet lanes)
    : hash_(std::move(hash))
    , log2_num_lanes_(log2_num_lanes)
    , lanes_(std::move(lanes))
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
      auto const result = executor.Execute(hash_, log2_num_lanes_, lanes_);

      status_ = result.status;
      fee_ += result.fee;
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
  using AtomicFee    = std::atomic<uint64_t>;

  TxDigest     hash_;
  uint32_t     log2_num_lanes_;
  LaneSet      lanes_;
  std::size_t  slice_;
  AtomicStatus status_{Status::NOT_RUN};
  AtomicFee    fee_{0};
};

}  // namespace ledger
}  // namespace fetch
