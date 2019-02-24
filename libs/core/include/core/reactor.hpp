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
#include "core/runnable.hpp"

#include <atomic>
#include <map>
#include <memory>
#include <thread>

namespace fetch {
namespace core {

class Reactor
{
public:
  static constexpr char const *LOGGING_NAME = "Reactor";

  // Construction / Destruction
  Reactor(std::string const &name);
  Reactor(Reactor const &) = delete;
  Reactor(Reactor &&)      = delete;
  ~Reactor()               = default;

  bool Attach(WeakRunnable runnable);
  bool Detach(Runnable const &runnable);

  void Start();
  void Stop();

  // Operators
  Reactor &operator=(Reactor const &) = delete;
  Reactor &operator=(Reactor &&) = delete;

private:
  using Mutex       = mutex::Mutex;
  using RunnableMap = std::map<Runnable const *, WeakRunnable>;
  using Flag        = std::atomic<bool>;
  using ThreadPtr   = std::unique_ptr<std::thread>;

  void StartWorker();
  void StopWorker();
  void Monitor();

  std::string const name_;
  Flag running_{false};

  Mutex       work_map_mutex_{__LINE__, __FILE__};
  RunnableMap work_map_{};

  Mutex     worker_mutex_{__LINE__, __FILE__};
  ThreadPtr worker_{};
};

}  // namespace core
}  // namespace fetch
