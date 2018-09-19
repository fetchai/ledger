#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "network/management/network_manager.hpp"

#include <iostream>
#include <string>

namespace fetch {
namespace generics {
template <typename WORKER>
class LifeTracker
{
  using p_target_type = std::mutex;
  using strong_p_type = std::shared_ptr<p_target_type>;
  using weak_p_type   = std::weak_ptr<p_target_type>;
  using mutex_type    = std::mutex;
  using lock_type     = std::lock_guard<mutex_type>;

public:
  LifeTracker(fetch::network::NetworkManager worker) : worker_(worker) {}

  void reset(void)
  {
    lock_type lock(*alive_);
    auto      alsoAlive = alive_;
    alive_.reset();
  }

  void Post(std::function<void(void)> func)
  {
    std::weak_ptr<std::mutex> deadOrAlive(alive_);
    auto                      cb = [deadOrAlive, func]() {
      auto aliveOrElse = deadOrAlive.lock();
      if (aliveOrElse)
      {
        lock_type lock(*aliveOrElse);
        func();
      }
    };
    worker_.Post(func);
  }

private:
  strong_p_type                  alive_ = std::make_shared<p_target_type>();
  fetch::network::NetworkManager worker_;
};

}  // namespace generics
}  // namespace fetch
