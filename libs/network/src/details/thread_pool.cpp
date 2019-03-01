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

#include "network/details/thread_pool.hpp"
#include "core/threading.hpp"

namespace fetch {
namespace network {
namespace details {

using std::chrono::milliseconds;
using std::this_thread::sleep_for;

/**
 * Create a thread pool instance with a specified number
 *
 * @param threads The maximum number of threads
 * @return
 */
ThreadPoolImplementation::ThreadPoolPtr ThreadPoolImplementation::Create(std::size_t        threads,
                                                                         std::string const &name)
{
  return std::make_shared<ThreadPoolImplementation>(threads, name);
}

/**
 * Construct the thread pool implementation
 *
 * @param threads The maximum number of threads
 */
ThreadPoolImplementation::ThreadPoolImplementation(std::size_t threads, std::string name)
  : max_threads_(threads)
  , name_(std::move(name))
{}

/**
 * Tear down the thread pool
 */
ThreadPoolImplementation::~ThreadPoolImplementation()
{
  Stop();
}

/**
 * Post a piece of work that will be executed in the near future
 *
 * @param item The work item to execute
 * @param milliseconds The (minimum) time to postpone the execution for
 */
void ThreadPoolImplementation::Post(WorkItem item, uint32_t milliseconds)
{
  if (!shutdown_)
  {
    future_work_.Post(std::move(item), milliseconds);

    FETCH_LOCK(idle_mutex_);
    work_available_.notify_one();
  }
}

/**
 * Post a piece of work to be executed
 *
 * @param item The work item to execute
 */
void ThreadPoolImplementation::Post(WorkItem item)
{
  if (!shutdown_)
  {
    work_.Post(std::move(item));

    FETCH_LOCK(idle_mutex_);
    work_available_.notify_one();
  }
}

/**
 * Update the idle / periodic execution interval
 *
 * @param milliseconds The new interval in milliseconds
 */
void ThreadPoolImplementation::SetIdleInterval(std::size_t milliseconds)
{
  idle_work_.SetInterval(milliseconds);
}

/**
 * Add a work item to the idle store
 *
 * @param idle_work The work to the executed periodically
 */
void ThreadPoolImplementation::PostIdle(WorkItem idle_work)
{
  if (!shutdown_)
  {
    idle_work_.Post(std::move(idle_work));

    FETCH_LOCK(idle_mutex_);
    work_available_.notify_one();
  }
}

/**
 * Clear all the work queues
 */
void ThreadPoolImplementation::Clear()
{
  future_work_.Clear();
  idle_work_.Clear();
  work_.Clear();
}

/**
 * Start the thread pool.
 *
 * This is the point at which all the threads are created for this pool
 */
void ThreadPoolImplementation::Start()
{
  static constexpr std::size_t MAX_START_LOOPS     = 30;
  static constexpr std::size_t START_LOOP_INTERVAL = 100;

  if (shutdown_)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Thread pool may not be restarted after it has been shutdown");
    return;
  }

  // start all the threads
  {
    FETCH_LOCK(threads_mutex_);

    if (!threads_.empty())
    {
      throw std::runtime_error("Attempting to start the thread pool multiple times");
    }

    for (std::size_t thread_idx = 0; thread_idx < max_threads_; ++thread_idx)
    {
      threads_.emplace_back(
          std::make_shared<std::thread>([this, thread_idx]() { this->ProcessLoop(thread_idx); }));
    }
  }

  // wait for the threads to start
  bool success = false;
  for (std::size_t loop = 0; loop < MAX_START_LOOPS; ++loop)
  {
    if (inactive_threads_ >= max_threads_)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Started on loop: ", loop);
      success = true;
      break;
    }

    sleep_for(milliseconds{START_LOOP_INTERVAL});
  }

  if (!success)
  {
    throw std::runtime_error("Unable to start threads in the requested time period");
  }
}

/**
 * Stop the thread pool
 *
 * This is when the internal threads will be removed
 */
void ThreadPoolImplementation::Stop()
{
  FETCH_LOCK(threads_mutex_);

  // We have made the design decision that we will not allow pooled work to stop the thread pool.
  // While strictly not necessary, this has been done as a guard against desired behaviour. If this
  // assumption should prove to be invalid in the future removing this check here should result in
  // full functioning code
  for (auto &thread : threads_)
  {
    if (std::this_thread::get_id() == thread->get_id())
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Thread pools must not be killed by a thread they own.");
      return;
    }
  }

  // signal to all the working threads that they should immediately stop all further work
  shutdown_ = true;
  future_work_.Abort();
  idle_work_.Abort();
  work_.Abort();

  {
    // kick all the threads to start wake and
    FETCH_LOCK(idle_mutex_);
    work_available_.notify_all();
  }

  // wait for all the threads to conclude
  for (auto &thread : threads_)
  {
    // do not join ourselves
    if (std::this_thread::get_id() != thread->get_id())
    {
      thread->join();
    }
  }

  // delete all the thread instances
  threads_.clear();

  // clear all the work items inside the respective queues
  future_work_.Clear();
  idle_work_.Clear();
  work_.Clear();
}

/**
 * The internal entrypoint for all dispatch threads used in the thread pool.
 *
 * @param index The index of the thread (used for debug purposes)
 * @private
 */
void ThreadPoolImplementation::ProcessLoop(std::size_t index)
{
  SetThreadName("TP:" + name_, index);

  FETCH_LOG_DEBUG(LOGGING_NAME, "Creating thread pool worker (thread: ", index, ')');

  try
  {
    while (!shutdown_)
    {
      if (!Poll())
      {
        std::unique_lock<std::mutex> lock(idle_mutex_);

        // double check the emptiness of the queue because there is a race here
        if (!work_.IsEmpty())
        {
          FETCH_LOG_DEBUG(LOGGING_NAME, "Restarting the inactive thread (thread: ", index, " queue: ", name_, ')');
          continue;
        }

        FETCH_LOG_DEBUG(LOGGING_NAME, "Entering idle state (thread: ", index, ')');

        // snooze for a while or until more work arrives
        LOG_STACK_TRACE_POINT;

        // calculate the maximum time to wait. If the two queues are empty then this will be zero
        auto const next_future_item = future_work_.TimeUntilNextItem();
        auto const next_idle_cycle  = idle_work_.DueIn();
        auto const wait_time        = std::min(next_future_item, next_idle_cycle);

        // update the threading counters
        ++inactive_threads_;

        // wait for the next event
        if (wait_time == std::chrono::milliseconds::max())
        {
          // both the idle and future queues are empty, we need to wait for the next post event
          work_available_.wait(lock);
        }
        else if (wait_time != std::chrono::milliseconds::zero())
        {
          // the idle and/or future queues have items we therefore wait for (maximally) one of these
          // events.
          work_available_.wait_for(lock, wait_time);
        }

        --inactive_threads_;

        FETCH_LOG_DEBUG(LOGGING_NAME, "Exiting idle state (thread ", index, ')');
      }
    }
  }
  catch (std::exception &e)
  {
    FETCH_LOG_ERROR(LOGGING_NAME,
                    name_ + ": Thread_pool ProcessLoop is exiting, because: ", e.what());
    TODO_FAIL(name_ + ": ThreadPool: Should not get here!");
  }
  catch (...)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, name_ + ": Thread_pool ProcessLoop is exiting.");
    TODO_FAIL(name_ + ": ThreadPool: Should not get here!");
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Destroying thread pool worker (thread: ", index, ')');
}

/**
 * Periodic call made by dispatch threads to execute pending work in the queues
 *
 * @return false if the thread should enter an idle state next, otherwise true
 */
bool ThreadPoolImplementation::Poll()
{
  std::size_t count = 0;

  // dispatch any active tasks in the queue
  count += work_.Dispatch([this](WorkItem const &item) { ExecuteWorkload(item); });

  // allow early exit in abort / shutdowns
  if (shutdown_)
  {
    return true;
  }

  // enqueue future work if required
  count += future_work_.Dispatch([this](WorkItem const &item) { Post(item); });

  // allow early exit in abort / shutdowns
  if (shutdown_)
  {
    return true;
  }

  // trigger any required idle work (if it is time to do so)
  if (idle_work_.IsDue())
  {
    count += idle_work_.Visit([this](WorkItem const &item) { ExecuteWorkload(item); });
  }

  // update the global counter
  counter_ += count;

  return (count > 0);
}

/**
 * Wrapper around execution of a work item
 *
 * Catches exceptions and allows fast exit of the `shutdown_` flag has been set
 *
 * @param workload The work item to be executed
 * @return true on successful execution, otherwise false
 */
bool ThreadPoolImplementation::ExecuteWorkload(WorkItem const &workload)
{
  bool success = false;

  if (!shutdown_)
  {
    try
    {
      LOG_STACK_TRACE_POINT;

      // execute the item
      workload();

      // signal successful execution
      success = true;
    }
    catch (std::exception &ex)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Caught exception in ThreadPool::ExecuteWorkload - ",
                      ex.what());
    }
  }

  return success;
}

}  // namespace details
}  // namespace network
}  // namespace fetch
