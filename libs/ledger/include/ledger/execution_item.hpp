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

#include "core/bitvector.hpp"
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
  using LaneIndex  = uint32_t;
  using BlockIndex = ExecutorInterface::BlockIndex;
  using SliceIndex = ExecutorInterface::SliceIndex;
  using Status     = ExecutorInterface::Status;

  static constexpr char const *LOGGING_NAME = "ExecutionItem";

  // Construction / Destruction
  ExecutionItem(v2::Digest digest, BlockIndex block, SliceIndex slice, BitVector const &shards);
  ExecutionItem(ExecutionItem const &) = delete;
  ExecutionItem(ExecutionItem &&)      = delete;
  ~ExecutionItem()                     = default;

  /// @name Accessors
  /// @{
  v2::Digest const &digest() const;
  BitVector const & shards() const;
  Status            status() const;
  uint64_t          fee() const;
  /// @}

  void Execute(ExecutorInterface &executor);

  // Operators
  ExecutionItem &operator=(ExecutionItem const &) = delete;
  ExecutionItem &operator=(ExecutionItem &&) = delete;

private:
  using AtomicStatus = std::atomic<Status>;
  using AtomicFee    = std::atomic<uint64_t>;

  v2::Digest   digest_;
  BlockIndex   block_{0};
  SliceIndex   slice_{0};
  BitVector    shards_;
  AtomicStatus status_{Status::NOT_RUN};
  AtomicFee    fee_{0};
};

inline ExecutionItem::ExecutionItem(v2::Digest digest, BlockIndex block, SliceIndex slice,
                                    BitVector const &shards)
  : digest_(std::move(digest))
  , block_{block}
  , slice_{slice}
  , shards_(shards)
{}

inline v2::Digest const &ExecutionItem::digest() const
{
  return digest_;
}

inline BitVector const &ExecutionItem::shards() const
{
  return shards_;
}

inline ExecutionItem::Status ExecutionItem::status() const
{
  return status_;
}

inline uint64_t ExecutionItem::fee() const
{
  return fee_;
}

inline void ExecutionItem::Execute(ExecutorInterface &executor)
{
  try
  {
    auto const result = executor.Execute(digest_, block_, slice_, shards_);

    // update the internal results
    status_ = result.status;
    fee_ += result.fee;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Exception thrown while executing transaction: ", ex.what());

    status_ = Status::RESOURCE_FAILURE;
  }
}

}  // namespace ledger
}  // namespace fetch
