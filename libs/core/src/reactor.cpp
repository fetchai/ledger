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

#include "core/reactor.hpp"
#include "core/logger.hpp"
#include "core/runnable.hpp"
#include "core/threading.hpp"

#include <chrono>
#include <deque>

static const std::chrono::milliseconds POLL_INTERVAL{15};

using WorkQueue = std::deque<fetch::core::WeakRunnable>;

namespace fetch {
namespace core {

Reactor::Reactor(std::string name)
  : name_{std::move(name)}
{}

bool Reactor::Attach(WeakRunnable runnable)
{
  bool success{false};

  // convert to concrete runnable
  auto concrete_runnable = runnable.lock();
  if (concrete_runnable)
  {
    FETCH_LOCK(work_map_mutex_);

    // attempt to insert the element into the map
    auto const result = work_map_.insert(std::make_pair(concrete_runnable.get(), runnable));

    // signal success if the insertion was successful
    success = result.second;
  }

  return success;
}

bool Reactor::Detach(Runnable const &runnable)
{
  FETCH_LOCK(work_map_mutex_);
  return (work_map_.erase(&runnable) > 0);
}

void Reactor::Start()
{
  FETCH_LOCK(worker_mutex_);

  // restart the work if called multiple times
  StopWorker();
  StartWorker();
}

void Reactor::Stop()
{
  FETCH_LOCK(worker_mutex_);

  // stop the worker
  StopWorker();
}

void Reactor::StartWorker()
{
  if (worker_)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Worker already started, logic start up error");
    throw std::runtime_error("Worker already started");
  }

  // signal the reactor is running
  running_ = true;

  // create the worker routine
  worker_ = std::make_unique<std::thread>(&Reactor::Monitor, this);
}

void Reactor::StopWorker()
{
  running_ = false;

  if (worker_)
  {
    worker_->join();
    worker_.reset();
  }
}

void Reactor::Monitor()
{
  // set the thread name
  SetThreadName(name_);

  WorkQueue work_queue;

  while (running_)
  {
    // Step 1. If we have run out of work to execute then gather all the runnables that are ready
    if (work_queue.empty())
    {
      FETCH_LOCK(work_map_mutex_);

      // loop through and evaluate the map
      auto it = work_map_.begin();
      while (it != work_map_.end())
      {
        // attempt to lock the runnable
        auto concrete_runnable = it->second.lock();

        if (concrete_runnable)
        {
          // evaluate if the runnable is ready to execute, if it is then add it to the work queue
          if (concrete_runnable->IsReadyToExecute())
          {
            work_queue.emplace_back(it->second);
          }

          // advance to the next element in the map
          ++it;
        }
        else
        {
          // the lifetime of the runnable has expired, remove and advance to next element in the map
          it = work_map_.erase(it);
        }
      }
    }

    // If the work queue is still empty then there is no work to do. Sleep the worker and try again
    if (work_queue.empty())
    {
      std::this_thread::sleep_for(POLL_INTERVAL);
      continue;
    }

    // extract the element from the front of the queue
    auto runnable = work_queue.front().lock();
    work_queue.pop_front();

    // execute the item if it can be executed
    if (runnable)
    {
      runnable->Execute();
    }
  }
}

}  // namespace core
}  // namespace fetch