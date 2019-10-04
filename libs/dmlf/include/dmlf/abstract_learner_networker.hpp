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

#include "core/mutex.hpp"

#include "core/byte_array/byte_array.hpp"
#include "dmlf/queue.hpp"
#include "dmlf/queue_interface.hpp"
#include "dmlf/shuffle_algorithm_interface.hpp"
#include "dmlf/type_map.hpp"

namespace fetch {
namespace dmlf {

class AbstractLearnerNetworker
{
public:
  using Bytes = byte_array::ByteArray;

  AbstractLearnerNetworker()          = default;
  virtual ~AbstractLearnerNetworker() = default;

  // To implement
  virtual void        PushUpdate(const std::shared_ptr<UpdateInterface> &update) = 0;
  virtual std::size_t GetPeerCount() const                                       = 0;

  template <typename T>
  void Initialize()
  {
    FETCH_LOCK(queue_m_);
    if (!queue_)
    {
      queue_ = std::make_shared<Queue<T>>();
      return;
    }
    throw std::runtime_error{"Learner already initialized"};
  }

  std::size_t GetUpdateCount() const;

  template <typename T>
  std::shared_ptr<T> GetUpdate()
  {
    FETCH_LOCK(queue_m_);
    ThrowIfNotInitialized();
    auto que = std::dynamic_pointer_cast<Queue<T>>(queue_);
    return que->GetUpdate();
  }

  virtual void SetShuffleAlgorithm(const std::shared_ptr<ShuffleAlgorithmInterface> &alg);

  virtual void PushUpdateType(const std::string & /*key*/,
                              const std::shared_ptr<UpdateInterface> & /*update*/);

  template <typename T>
  void RegisterUpdateType(std::string key)
  {
    FETCH_LOCK(queue_map_m_);
    update_types_.template put<T>(key);
    queue_map_[key] = std::make_shared<Queue<T>>();
  }

  template <typename T>
  std::size_t GetUpdateTypeCount() const
  {
    FETCH_LOCK(queue_map_m_);
    auto key = update_types_.template find<T>();
    return queue_map_.at(key)->size();
  }

  std::size_t GetUpdateTypeCount(const std::string &key) const;

  template <typename T>
  std::shared_ptr<T> GetUpdateType()
  {
    FETCH_LOCK(queue_map_m_);
    auto key  = update_types_.template find<T>();
    auto iter = queue_map_.find(key);
    auto que  = std::dynamic_pointer_cast<Queue<T>>(iter->second);
    return que->GetUpdate();
  }

  AbstractLearnerNetworker(const AbstractLearnerNetworker &other) = delete;
  AbstractLearnerNetworker &operator=(const AbstractLearnerNetworker &other)  = delete;
  bool                      operator==(const AbstractLearnerNetworker &other) = delete;
  bool                      operator<(const AbstractLearnerNetworker &other)  = delete;

protected:
  std::shared_ptr<ShuffleAlgorithmInterface> alg_;  // used by descendents

  void NewMessage(const Bytes &msg);

  void NewDmlfMessage(const Bytes &msg);  // called by descendents

private:
  using Mutex             = fetch::Mutex;
  using Lock              = std::unique_lock<Mutex>;
  using QueueInterfacePtr = std::shared_ptr<QueueInterface>;
  using QueueInterfaceMap = std::unordered_map<std::string, QueueInterfacePtr>;

  QueueInterfacePtr queue_;
  mutable Mutex     queue_m_;
  QueueInterfaceMap queue_map_;
  mutable Mutex     queue_map_m_;

  TypeMap<> update_types_;

  void ThrowIfNotInitialized() const;
};

}  // namespace dmlf
}  // namespace fetch
