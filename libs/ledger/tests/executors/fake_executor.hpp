#pragma once

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
  struct HistoryElement
  {
    using clock_type     = std::chrono::high_resolution_clock;
    using timepoint_type = clock_type::time_point;

    HistoryElement(tx_digest_type const &h, std::size_t s, lane_set_type l)
      : hash(h), slice(s), lanes(std::move(l))
    {}
    HistoryElement(HistoryElement const &) = default;

    tx_digest_type hash;
    std::size_t    slice;
    lane_set_type  lanes;
    timepoint_type timestamp{clock_type::now()};
  };

  using history_cache_type = std::vector<HistoryElement>;
  using storage_type       = fetch::ledger::StorageInterface;

  Status Execute(tx_digest_type const &hash, std::size_t slice, lane_set_type const &lanes) override
  {
    history_.emplace_back(hash, slice, lanes);

    // if we have a state then make some changes to it
    if (state_)
    {
      state_->Set(fetch::storage::ResourceAddress{hash}, "executed");
    }

    return Status::SUCCESS;
  }

  std::size_t GetNumExecutions() const { return history_.size(); }

  void CollectHistory(history_cache_type &history)
  {
    history_.reserve(history.size() + history_.size());  // do the allocation
    history.insert(history.end(), history_.begin(), history_.end());
  }

  void SetStorageInterface(storage_type &state) { state_ = &state; }

  void ClearStorageInterface() { state_ = nullptr; }

private:
  storage_type *     state_ = nullptr;
  history_cache_type history_;
};
