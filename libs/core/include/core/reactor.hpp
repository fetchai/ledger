#pragma once
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

#include "core/runnable.hpp"
#include "core/synchronisation/protected.hpp"
#include "telemetry/telemetry.hpp"

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>

namespace fetch {
namespace core {

class Reactor
{
public:
  // Construction / Destruction
  explicit Reactor(std::string name);
  Reactor(Reactor const &) = delete;
  Reactor(Reactor &&)      = delete;
  ~Reactor();

  bool Attach(WeakRunnable runnable);   // NOLINT
  bool Attach(WeakRunnables runnable);  // NOLINT
  bool Detach(Runnable const &runnable);

  void Start();
  void Stop();

  // Operators
  Reactor &operator=(Reactor const &) = delete;
  Reactor &operator=(Reactor &&) = delete;

private:
  using RunnableMap     = Protected<std::map<Runnable const *, WeakRunnable>>;
  using Flag            = std::atomic<bool>;
  using ProtectedThread = Protected<std::thread>;
  using ThreadPtr       = std::unique_ptr<ProtectedThread>;

  void StartWorker();
  void StopWorker();
  void Monitor();

  telemetry::HistogramPtr       CreateHistogram(char const *name, char const *description) const;
  telemetry::CounterPtr         CreateCounter(char const *name, char const *description) const;
  telemetry::GaugePtr<uint64_t> CreateGauge(char const *name, char const *description) const;

  std::string const name_;
  Flag              running_{false};

  RunnableMap work_map_{};
  ThreadPtr   worker_{};

  // telemetry
  telemetry::HistogramPtr       runnables_time_;
  telemetry::CounterPtr         attach_total_;
  telemetry::CounterPtr         detach_total_;
  telemetry::CounterPtr         runnable_total_;
  telemetry::CounterPtr         sleep_total_;
  telemetry::CounterPtr         success_total_;
  telemetry::CounterPtr         failure_total_;
  telemetry::CounterPtr         expired_total_;
  telemetry::GaugePtr<uint64_t> work_queue_length_;
  telemetry::GaugePtr<uint64_t> work_queue_max_length_;
};

}  // namespace core
}  // namespace fetch
