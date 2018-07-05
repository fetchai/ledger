#ifndef FETCH_EXECUTION_MANAGER_HPP
#define FETCH_EXECUTION_MANAGER_HPP

#include "ledger/executor.hpp"
#include "ledger/execution_manager_interface.hpp"
#include "network/details/thread_pool.hpp"
#include "core/mutex.hpp"

#include <memory>
#include <vector>
#include <unordered_set>
#include <atomic>

namespace fetch {
namespace ledger {

struct ExecutionItem {
  using lane_index_type = uint16_t;
  using tx_digest_type = chain::Transaction::digest_type;
  using lane_set_type = std::unordered_set<lane_index_type>;

  tx_digest_type hash;
  lane_set_type lanes;
  std::size_t slice;
};


/**
 * The Execution Manager is the object which orchestrates the execution of a
 * specified block across a series of executors and lanes.
 */
class ExecutionManager : public ExecutionManagerInterface,
                         public std::enable_shared_from_this<ExecutionManager> {
public:
  using shared_executor_type = std::shared_ptr<Executor>;
  using executor_list_type = std::vector<shared_executor_type>;
  using block_slices_type = std::vector<chain::BlockSlice>;
  using block_digest_type = byte_array::ConstByteArray;

  using execution_plan_type = std::vector<ExecutionItem>;
  using thread_pool_type = std::shared_ptr<fetch::network::ThreadPool>;
  using mutex_type = fetch::mutex::Mutex;
  using counter_type = std::atomic<std::size_t>;
  using flag_type = std::atomic<bool>;

  // Construction / Destruction
  explicit ExecutionManager(std::size_t num_executors);

  // main interface
  bool Execute(tx_index_type const &index,
               block_map_type &map,
               std::size_t num_lanes,
               std::size_t num_slices) override;

  // general control of the operation of the module
  void Start();
  void Stop();

  // statistics
  std::size_t completed_executions() const {
    return completed_executions_;
  }

private:

  flag_type running_{false};
  mutex_type idle_executors_lock_;
  executor_list_type idle_executors_;
  counter_type active_count_{0};
  counter_type completed_executions_{0};
  thread_pool_type thread_pool_;

  void DispatchExecution(ExecutionItem const &item);
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTION_MANAGER_HPP
