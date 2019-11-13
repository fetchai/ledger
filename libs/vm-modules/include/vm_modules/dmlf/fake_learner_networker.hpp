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

#include <memory>
#include <queue>
#include <stdexcept>

#include "core/mutex.hpp"

#include "math/tensor.hpp"

namespace fetch {
namespace dmlf {

template <typename T>
class FakeLearner
{
public:
  using UpdateType    = T;
  using UpdateTypePtr = std::shared_ptr<UpdateType>;

  FakeLearner()                         = default;
  virtual ~FakeLearner()                = default;
  FakeLearner(FakeLearner const &other) = delete;
  FakeLearner &operator=(FakeLearner const &other) = delete;

  void PushUpdate(UpdateTypePtr const &update)
  {
    FETCH_LOCK(queue_m_);
    queue_.push(update);
  }

  std::size_t GetPeerCount() const
  {
    return 0;
  }

  std::size_t GetUpdateCount() const
  {
    return queue_.size();
  }

  UpdateTypePtr GetUpdate()
  {
    FETCH_LOCK(queue_m_);
    if (queue_.empty())
    {
      throw std::runtime_error{"FakeLearner: no updates available"};
    }
    UpdateTypePtr update = queue_.front();
    queue_.pop();
    return update;
  }

private:
  using Mutex = fetch::Mutex;
  using Lock  = std::unique_lock<Mutex>;
  using Queue = std::queue<UpdateTypePtr>;

  Queue         queue_;
  mutable Mutex queue_m_;
};

}  // namespace dmlf
}  // namespace fetch
