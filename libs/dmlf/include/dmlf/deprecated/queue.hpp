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

#include "core/mutex.hpp"

#include "dmlf/deprecated/queue_interface.hpp"
#include "dmlf/deprecated/update.hpp"
#include "dmlf/deprecated/update_interface.hpp"

namespace fetch {
namespace dmlf {

template <typename T>
class Queue : public QueueInterface
{
public:
  using UpdateType = T;

  using UpdatePtr    = std::shared_ptr<UpdateType>;
  using StoredType   = std::pair<UpdatePtr, Bytes>;
  using QueueUpdates = std::priority_queue<StoredType, std::vector<StoredType>, std::greater<>>;

  Queue()           = default;
  ~Queue() override = default;

  void PushNewMessage(Bytes msg) override
  {
    auto update = std::make_shared<UpdateType>();
    update->DeSerialise(msg);
    {
      FETCH_LOCK(updates_m_);
      updates_.push({update, msg});
    }
  }

  std::size_t size() const override
  {
    FETCH_LOCK(updates_m_);
    return updates_.size();
  }

  Bytes PopAsBytes() override
  {
    FETCH_LOCK(updates_m_);

    auto b = PeekAsBytes();
    Drop();
    return b;
  }

  Bytes PeekAsBytes() override
  {
    FETCH_LOCK(updates_m_);
    if (!updates_.empty())
    {
      auto res = updates_.top().second;
      return res;
    }
    throw std::length_error{"Updates queue is empty"};
  }

  void Drop() override
  {
    FETCH_LOCK(updates_m_);
    if (!updates_.empty())
    {
      updates_.pop();
    }
    throw std::length_error{"Updates queue is empty"};
  }

  std::shared_ptr<UpdateType> GetUpdate()
  {
    FETCH_LOCK(updates_m_);
    if (!updates_.empty())
    {
      auto upd = updates_.top().first;
      updates_.pop();
      return upd;
    }
    throw std::length_error{"Updates queue is empty"};
  }

  Queue(Queue const &other) = delete;
  Queue &operator=(Queue const &other)  = delete;
  bool   operator==(Queue const &other) = delete;
  bool   operator<(Queue const &other)  = delete;

private:
  using Mutex = fetch::Mutex;
  using Lock  = std::unique_lock<Mutex>;

  QueueUpdates  updates_;
  mutable Mutex updates_m_;
};

}  // namespace dmlf
}  // namespace fetch
