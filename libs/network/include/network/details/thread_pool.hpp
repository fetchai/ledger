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

#include "core/mutex.hpp"
#include "network/details/future_work_store.hpp"
#include "network/details/idle_work_store.hpp"
#include "network/details/work_store.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fetch {
namespace network {
namespace details {

/**
 * @brief Application thread pool for dispatching work
 *
 * The application thread pool at a conceptual level is a simple set queue of ordered
 * work queues.
 *
 * The main work queue is a FIFO based model and these jobs are extracted by the dispatch
 * threads.
 *
 * The other work queue is the future work queue. These jobs are ordered by due time and
 * once the due time has been reached they are placed at the end of the work queue. Users
 * should note that the due timestamp can be thought of as the mimimum schedule time.
 *
 * The third queue is an "idle" work store. This is probably better thought of as a
 * periodic or reoccuring work. Work from this store is executed directly. The design of
 * the thread pool assumes that the number of such tasks will be relatively small and that
 * execution of these items will be relatively short. If this is not the case throughput
 * performance might be affected.
 *
 *
 *        ┌────────────────────┐
 *        │ Future Work Queue  │──┐
 *        └────────────────────┘  │
 *                                │
 *     ┌──────────────────────────┘
 *     │
 *     │  ┌────────────────────┐
 *     └─▶│     Work Queue     │ ──────┐
 *        └────────────────────┘       │       ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
 *                                     │                              │
 *                                     ├──────▶│   Dispatch Threads
 *                                     │                              │
 *        ┌────────────────────┐       │       └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
 *        │  Idle Work Store   │ ──────┘
 *        └────────────────────┘
 */
class ThreadPoolImplementation : public std::enable_shared_from_this<ThreadPoolImplementation>
{
public:
  static constexpr char const *LOGGING_NAME = "ThreadPoolImpl";

  using ThreadPoolPtr = std::shared_ptr<ThreadPoolImplementation>;
  using WorkItem      = std::function<void()>;

  static ThreadPoolPtr Create(std::size_t threads, std::string const &name);

  explicit ThreadPoolImplementation(std::size_t threads, std::string name);
  ThreadPoolImplementation(ThreadPoolImplementation const &) = delete;
  ThreadPoolImplementation(ThreadPoolImplementation &&)      = delete;
  ~ThreadPoolImplementation();

  /// @name Current / Future Work
  /// @{
  void Post(WorkItem work, uint32_t milliseconds);
  void Post(WorkItem work);
  /// @}

  /// @name Idle / Background tasks
  /// @{
  void PostIdle(WorkItem work);
  void SetIdleInterval(std::size_t milliseconds);
  /// @}

  /// @name Thread pool control
  /// @{
  void Start();
  void Stop();
  void Clear();
  /// @}

  std::size_t execute_count() const
  {
    return counter_;
  }

  // Operators
  ThreadPoolImplementation &operator=(ThreadPoolImplementation const &) = delete;
  ThreadPoolImplementation &operator=(ThreadPoolImplementation &&) = delete;

private:
  using Mutex      = fetch::mutex::Mutex;
  using ThreadPtr  = std::shared_ptr<std::thread>;
  using ThreadPool = std::vector<ThreadPtr>;
  using Flag       = std::atomic<bool>;
  using Counter    = std::atomic<std::size_t>;
  using Condition  = std::condition_variable;

  void ProcessLoop(std::size_t index);

  bool Poll();
  bool ExecuteWorkload(WorkItem const &workload);

  std::size_t const max_threads_ = 1;  ///< Config: Max number of threads

  mutable Mutex threads_mutex_{__LINE__, __FILE__};  ///< Mutex protecting the thread store
  ThreadPool    threads_;                            ///< Container of threads

  WorkStore       work_;         ///< The main work queue
  FutureWorkStore future_work_;  ///< The future work queue
  IdleWorkStore   idle_work_;    ///< The idle work store

  Condition     work_available_;                  ///< Work available condition
  mutable Mutex idle_mutex_{__LINE__, __FILE__};  ///< Associated mutex for condition
  Flag          shutdown_{false};                 ///< Flag to signal the pool should stop
  Counter       counter_{0};                      ///< The number of jobs executed
  Counter       inactive_threads_{0};             ///< The number of threads waiting for work

  std::string name_{};
};

}  // namespace details

using ThreadPool = typename std::shared_ptr<details::ThreadPoolImplementation>;

inline ThreadPool MakeThreadPool(std::size_t threads = 1, std::string const &name = "")
{
  return details::ThreadPoolImplementation::Create(threads, name);
}

}  // namespace network
}  // namespace fetch
