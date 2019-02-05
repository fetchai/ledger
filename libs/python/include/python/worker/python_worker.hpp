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

#include "network/generics/network_node_core.hpp"
#include "network/swarm/swarm_agent_api_impl.hpp"
#include "network/swarm/swarm_http_interface.hpp"
#include "network/swarm/swarm_random.hpp"

#include <string>
#include <time.h>
#include <unistd.h>

namespace fetch {
namespace swarm {

class PythonWorker
{
public:
  using mutex_type = std::recursive_mutex;
  using lock_type  = std::lock_guard<std::recursive_mutex>;

  virtual void Start()
  {
    lock_type lock(mutex_);
    tm_->Start();
  }

  virtual void Stop()
  {
    lock_type lock(mutex_);
    tm_->Stop();
  }

  void UseCore(std::shared_ptr<fetch::network::NetworkNodeCore> nn_core)
  {
    nn_core_ = nn_core;
  }

  template <typename F>
  void Post(F &&f)
  {
    tm_->Post(f);
  }
  template <typename F>
  void Post(F &&f, uint32_t milliseconds)
  {
    tm_->Post(f, milliseconds);
  }

  PythonWorker()
  {
    tm_ = fetch::network::MakeThreadPool(1, "PythonWorker");
  }

  virtual ~PythonWorker()
  {
    Stop();
  }

private:
  mutex_type                                       mutex_;
  fetch::network::ThreadPool                       tm_;
  std::shared_ptr<fetch::network::NetworkNodeCore> nn_core_;
};

}  // namespace swarm
}  // namespace fetch
