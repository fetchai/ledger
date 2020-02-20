//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/assert.hpp"
#include "core/reactor.hpp"
#include "core/runnable.hpp"
#include "core/set_thread_name.hpp"
#include "logging/logging.hpp"
#include "moment/deadline_timer.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

#include <chrono>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

static const std::chrono::milliseconds POLL_INTERVAL{15};
static constexpr char const *          LOGGING_NAME = "Reactor";

using WorkQueue = std::deque<fetch::core::WeakRunnable>;

namespace fetch {
namespace core {

Reactor::Reactor(std::string name)
  : name_{std::move(name)}
  , runnables_time_{CreateHistogram("ledger_reactor_runnable_time",
                                    "The histogram of runnables execution time")}
  , attach_total_{CreateCounter("ledger_reactor_attach_total",
                                "The total number of times a runnable was attached to the reactor")}
  , detach_total_{CreateCounter(
        "ledger_reactor_detach_total",
        "The total number of times a runnable was detached from the reactor")}
  , runnable_total_{CreateCounter("ledger_reactor_runnables_total",
                                  "The total number of runnables processed")}
  , sleep_total_{CreateCounter("ledger_reactor_sleep_total",
                               "The total number of times the reactor has slept")}
  , success_total_{CreateCounter(
        "ledger_reactor_success_total",
        "The total number of times the reactor has successfully executed a runable")}
  , failure_total_{CreateCounter(
        "ledger_reactor_failure_total",
        "The total number of times the reactor has failed to execute a runnable")}
  , expired_total_{CreateCounter("ledger_reactor_expired_total",
                                 "The total number of expired runnables")}
  , too_long_total_{CreateCounter("ledger_reactor_too_long_total",
                                  "The total number of runnables that took too long")}
  , way_too_long_total_{CreateCounter("ledger_reactor_way_too_long_total",
                                      "The total number of runnables that took way too long")}
  , work_queue_length_{CreateGauge("ledger_reactor_work_queue_length",
                                   "The current size of the work queue")}
  , work_queue_max_length_{
        CreateGauge("ledger_reactor_max_work_queue_length", "The max size of the work queue")}
{}

bool Reactor::Attach(WeakRunnable runnable)
{
  bool success{false};

  // convert to concrete runnable
  auto concrete_runnable = runnable.lock();
  if (concrete_runnable)
  {
    success = work_map_.Apply([&concrete_runnable, &runnable](auto &work_map) -> bool {
      // attempt to insert the element into the map
      auto const result = work_map.insert(std::make_pair(concrete_runnable.get(), runnable));

      // signal success if the insertion was successful
      return result.second;
    });
  }

  attach_total_->increment();

  return success;
}

bool Reactor::Attach(std::vector<WeakRunnable> runnables)
{
  bool success{false};
  for (auto const &runnable : runnables)
  {
    success = Attach(runnable);

    if (!success)
    {
      return false;
    }
  }

  return true;
}

bool Reactor::Detach(Runnable const &runnable)
{
  detach_total_->increment();
  return work_map_.Apply(
      [&runnable](auto &work_map) -> bool { return work_map.erase(&runnable) > 0; });
}

void Reactor::Start()
{
  // restart the work if called multiple times
  StopWorkerAndWatcher();
  StartWorkerAndWatcher();
}

void Reactor::Stop()
{
  // stop the worker
  StopWorkerAndWatcher();
}

void Reactor::StartWorkerAndWatcher()
{
  detailed_assert(!worker_);
  detailed_assert(!watcher_);

  // signal the reactor is running
  running_ = true;

  // create the worker routine
  worker_ = std::make_unique<ProtectedThread>(&Reactor::Monitor, this);

  // The reactor watcher determines whether executions are taking
  // too long or are stalled.
  watcher_ = std::make_unique<ProtectedThread>(&Reactor::ReactorWatch, this);
}

Reactor::~Reactor()
{
  StopWorkerAndWatcher();
}

void Reactor::StopWorkerAndWatcher()
{
  running_ = false;

  if (watcher_)
  {
    cv_.notify_all();  // Force the watcher awake
    watcher_->ApplyVoid([](auto &watcher) { watcher.join(); });
    watcher_.reset();
  }

  if (worker_)
  {
    worker_->ApplyVoid([](auto &worker) { worker.join(); });
    worker_.reset();
  }
}

void Reactor::ReactorWatch()
{
  uint32_t last_seen_executed = 0;

  while (running_)
  {
    std::unique_lock<std::mutex> lock(cv_m_);
    cv_.wait_for(lock, std::chrono::milliseconds(thread_watcher_check_ms_));

    auto        runnable_concrete = last_executed_runnable_.lock();
    std::string runnable_name     = runnable_concrete ? runnable_concrete->GetId() : "nullptr fail";
    std::string runnable_debug = runnable_concrete ? runnable_concrete->GetDebug() : "nullptr fail";

    if ((last_seen_executed == execution_counter_) && currently_executing_)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Very long execution noticed at execution counter: ", last_seen_executed,
                     ". from runnable: ", runnable_name, " debug: ", runnable_debug);
      executions_way_too_long_++;
      way_too_long_total_->increment();
    }

    last_seen_executed = execution_counter_;
  }
}

void Reactor::Monitor()
{
  // set the thread name
  SetThreadName(name_);

  std::string const     timer_name = "reactor:" + name_;
  moment::DeadlineTimer execution_too_long_timer{timer_name.c_str()};

  WorkQueue work_queue;

  while (running_)
  {
    // Step 1. If we have run out of work to execute then gather all the runnables that are ready
    if (work_queue.empty())
    {
      work_map_.ApplyVoid([&work_queue, this](auto &work_map) {
        // loop through and evaluate the map
        auto it = work_map.begin();
        while (it != work_map.end())
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
            // the lifetime of the runnable has expired, remove
            // and advance to next element in the map
            it = work_map.erase(it);

            expired_total_->increment();
          }
        }
      });

      work_queue_max_length_->max(work_queue.size());
    }

    work_queue_length_->set(work_queue.size());

    // If the work queue is still empty then there is no work to do. Sleep the worker and try again
    if (work_queue.empty())
    {
      sleep_total_->increment();
      std::this_thread::sleep_for(POLL_INTERVAL);

      continue;
    }

    // extract the element from the front of the queue, keep note of it for block detection
    execution_counter_++;
    last_executed_runnable_ = work_queue.front();

    auto runnable = last_executed_runnable_.lock();
    work_queue.pop_front();

    // execute the item if it can be executed
    if (runnable)
    {
      telemetry::FunctionTimer timer{*runnables_time_};
      runnable_total_->increment();

      try
      {
        currently_executing_ = true;
        execution_too_long_timer.Restart(execution_too_long_ms_);

        runnable->Execute();

        success_total_->increment();

        if (execution_too_long_timer.HasExpired())
        {
          FETCH_LOG_WARN(LOGGING_NAME,
                         "Execution took longer than was polite! From: ", runnable->GetId(),
                         " Debug: ", runnable->GetDebug());
          executions_too_long_++;
          too_long_total_->increment();
        }
      }
      catch (std::exception const &ex)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "The reactor ", name_, " caught an exception in ",
                       runnable->GetId(), "! ", " error: ", ex.what(),
                       " Debug: ", runnable->GetDebug());

        failure_total_->increment();
      }
      catch (...)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Unknown error generated in reactor: ", name_,
                       " From: ", runnable->GetId(), " Debug: ", runnable->GetDebug());

        failure_total_->increment();
      }

      currently_executing_ = false;
    }
  }
}

telemetry::HistogramPtr Reactor::CreateHistogram(char const *name, char const *description) const
{
  return telemetry::Registry::Instance().CreateHistogram(
      {0.000000001, 0.00000001, 0.0000001, 0.000001, 0.00001, 0.0001, 0.001, 0.01, 0.1, 1.0, 10.0},
      name, description, {{"reactor", name_}});
}

telemetry::CounterPtr Reactor::CreateCounter(char const *name, char const *description) const
{
  return telemetry::Registry::Instance().CreateCounter(name, description, {{"reactor", name_}});
}

telemetry::GaugePtr<uint64_t> Reactor::CreateGauge(char const *name, char const *description) const
{
  return telemetry::Registry::Instance().CreateGauge<uint64_t>(name, description,
                                                               {{"reactor", name_}});
}

uint64_t &Reactor::ExecutionTooLongMs()
{
  return execution_too_long_ms_;
}

uint64_t &Reactor::ThreadWatcherCheckMs()
{
  return thread_watcher_check_ms_;
}

uint32_t Reactor::ExecutionsTooLongCounter() const
{
  return executions_too_long_;
}

uint32_t Reactor::ExecutionsWayTooLongCounter() const
{
  return executions_way_too_long_;
}

}  // namespace core
}  // namespace fetch
