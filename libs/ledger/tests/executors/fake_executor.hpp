#ifndef FETCH_FAKE_EXECUTOR_HPP
#define FETCH_FAKE_EXECUTOR_HPP

#include "ledger/executor_interface.hpp"
#include "core/logger.hpp"

#include <chrono>
#include <atomic>
#include <iostream>
#include <utility>
#include <sstream>
#include <thread>

class FakeExecutor : public fetch::ledger::ExecutorInterface {
public:
  struct HistoryElement {
    using clock_type = std::chrono::high_resolution_clock;
    using timepoint_type = clock_type::time_point;

    HistoryElement(tx_digest_type const &h, std::size_t s, lane_set_type l)
      : hash(h)
      , slice(s)
      , lanes(std::move(l)) {
    }
    HistoryElement(HistoryElement const &) = default;

    tx_digest_type hash;
    std::size_t slice;
    lane_set_type lanes;
    timepoint_type timestamp{clock_type::now()};
  };

  using history_cache_type = std::vector<HistoryElement>;

  Status Execute(tx_digest_type const &hash, std::size_t slice, lane_set_type const &lanes) override {
    history_.emplace_back(hash, slice, lanes);

#if 0
    // format the message
    std::ostringstream oss;
    oss << "Executing transaction for slice: " << slice << " lanes: ";
    bool first_loop = true;
    for (auto lane : lanes) {
      if (!first_loop) oss << ", ";
      oss << lane;
      first_loop = false;
    }
    fetch::logger.Info(oss.str());
#endif

    return Status::SUCCESS;
  }

  std::size_t GetNumExecutions() const {
    return history_.size();
  }

  void CollectHistory(history_cache_type &history) {
    history_.reserve(history.size() + history_.size()); // do the allocation
    history.insert(history.end(), history_.begin(), history_.end());
  }

private:

  history_cache_type history_;
};

#endif //FETCH_FAKE_EXECUTOR_HPP
