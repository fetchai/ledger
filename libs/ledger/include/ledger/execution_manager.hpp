#ifndef FETCH_EXECUTION_MANAGER_HPP
#define FETCH_EXECUTION_MANAGER_HPP

#include "ledger/executor.hpp"
#include "ledger/execution_manager_interface.hpp"
#include "network/details/thread_pool.hpp"
#include "core/mutex.hpp"
#include "core/make_unique.hpp"

#include <memory>
#include <vector>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <future>
#include <condition_variable>

namespace fetch {
namespace ledger {

class ExecutionItem {
public:
  using lane_index_type = uint16_t;
  using tx_digest_type = chain::Transaction::digest_type;
  using lane_set_type = std::unordered_set<lane_index_type>;
  using promise_type = std::promise<Executor::Status>;
  using promise_ptr_type = std::unique_ptr<promise_type>;
  using underlying_future_type = std::future<Executor::Status>;
  using future_type = std::shared_ptr<underlying_future_type>;

  ExecutionItem(tx_digest_type const &hash, lane_index_type lane, std::size_t slice)
    : hash_(hash), lanes_{lane}, slice_(slice), promise_(fetch::make_unique<promise_type>()) {
  }

  ExecutorInterface::Status Execute(ExecutorInterface &executor) {
    return executor.Execute(hash_, slice_, lanes_);
  }

  void AddLane(lane_index_type lane) {
    lanes_.insert(lane);
  }

  future_type GetFuture() {
    return std::make_shared<underlying_future_type>(promise_->get_future());
  }

private:

  tx_digest_type hash_;
  lane_set_type lanes_;
  std::size_t slice_;
  promise_ptr_type promise_;
};


/**
 * The Execution Manager is the object which orchestrates the execution of a
 * specified block across a series of executors and lanes.
 */
class ExecutionManager : public ExecutionManagerInterface,
                         public std::enable_shared_from_this<ExecutionManager> {
public:
  using shared_executor_type = std::shared_ptr<ExecutorInterface>;
  using executor_list_type = std::vector<shared_executor_type>;
  using block_slices_type = std::vector<chain::BlockSlice>;
  using block_digest_type = byte_array::ConstByteArray;

  using execution_item_type = std::unique_ptr<ExecutionItem>;
  using execution_list_type = std::vector<execution_item_type>;
  using execution_plan_type = std::vector<execution_list_type>;

  using thread_pool_type = std::shared_ptr<fetch::network::ThreadPool>;
  using mutex_type = fetch::mutex::Mutex;
  using counter_type = std::atomic<std::size_t>;
  using flag_type = std::atomic<bool>;
  using thread_type = std::unique_ptr<std::thread>;
  using executor_factory_type = std::function<shared_executor_type()>;

  // Construction / Destruction
  explicit ExecutionManager(std::size_t num_executors, executor_factory_type const &factory);

  /// @name Execution Manager Interface
  /// @{
  bool Execute(block_digest_type const &block_hash,
               block_digest_type const &prev_block_hash,
               tx_index_type const &index,
               block_map_type &map,
               std::size_t num_lanes,
               std::size_t num_slices) override;

  block_digest_type LastProcessedBlock() override;
  bool IsActive() override;
  bool IsIdle() override;
  bool Abort() override;
  /// @}

  // general control of the operation of the module
  void Start();
  void Stop();

  // statistics
  std::size_t completed_executions() const {
    return completed_executions_;
  }

private:

  flag_type running_{false};
  flag_type active_{false};
  flag_type monitor_ready_{false};

  execution_plan_type execution_plan_;
  mutex_type execution_plan_lock_;
  block_digest_type last_block_hash_;

  std::condition_variable monitor_wake_;
  std::condition_variable monitor_notify_;

  mutex_type idle_executors_lock_;
  executor_list_type idle_executors_;

  counter_type active_count_{0};
  counter_type completed_executions_{0};
  counter_type remaining_executions_{0};
  thread_pool_type thread_pool_;
  thread_type monitor_thread_;

  //std::size_t slice_index_{0};
  std::atomic<std::size_t> num_slices_;

  enum class MonitorState {
    IDLE,
    SCHEDULE_NEXT_SLICE,
    RUNNING
  };

  // TODO: (EJF) for debug at the moment, needs to be moved (back) into thread stack
  std::atomic<MonitorState> monitor_state_{MonitorState::IDLE};


  void MonitorThreadEntrypoint();
  void PlanExecution(tx_index_type const &index,
                     block_map_type &map,
                     std::size_t num_lanes,
                     std::size_t num_slices);
  void ScheduleExecution();
  void DispatchExecution(ExecutionItem &item);

  void NotifyComplete();
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTION_MANAGER_HPP
