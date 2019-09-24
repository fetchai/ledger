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

//#include "core/byte_array/byte_array.hpp"
//#include "dmlf/ishuffle_algorithm.hpp"
#include "dmlf/ilearner_networker.hpp"
#include "dmlf/iqueue.hpp"
#include "dmlf/queue.hpp"

namespace fetch {
namespace dmlf {

//class AbstractLearnerNetworker: public ILearnerNetworker
class AbstractLearnerNetworker
{
public:
  using Intermediate = ILearnerNetworker::Intermediate;


  AbstractLearnerNetworker()
  {}
  virtual ~AbstractLearnerNetworker()
  {}
  
  template <typename T>
  void Initialize()
  {
    Lock l{queue_m_};
    if(!queue_)
    { 
      queue_ = std::make_shared<Queue<T>>();
      return;
    }
    throw std::runtime_error{"Learner already initialized"};
  }
  
  std::size_t getUpdateCount() const
  {
    Lock l{queue_m_};
    return queue_->size(); 
  }
  
  template <typename T>
  std::shared_ptr<T> getUpdate()
  {
    Lock l{queue_m_};
    auto que = std::dynamic_pointer_cast<Queue<T>>(queue_);
    return que->getUpdate();
  }

  virtual void        pushUpdate(std::shared_ptr<IUpdate> update) = 0;
  virtual std::size_t getPeerCount() const                        = 0;

  /*

  virtual void        pushUpdate(std::shared_ptr<IUpdate> update) = 0;
  virtual std::size_t getUpdateCount() const                      = 0;
  virtual std::size_t getPeerCount() const                        = 0;

  template <class UPDATE_TYPE>
  std::shared_ptr<UPDATE_TYPE> getUpdate()
  {
    auto r = std::make_shared<UPDATE_TYPE>();
    r->deserialise(getUpdateIntermediate());
    return r;
  }

  */

protected:
  /*
  std::shared_ptr<IShuffleAlgorithm> alg;                          // used by descendents
  virtual Intermediate               getUpdateIntermediate() = 0;  // implemented by descendents
  */
  void NewMessage(const Intermediate& msg)
  {
    Lock l{queue_m_};
    queue_->PushNewMessage(msg);
  }

private:
  using Mutex            = fetch::Mutex;
  using Lock             = std::unique_lock<Mutex>;
  
  std::shared_ptr<IQueue> queue_;
  mutable Mutex queue_m_;
  
  AbstractLearnerNetworker(const AbstractLearnerNetworker &other) = delete;
  AbstractLearnerNetworker &operator=(const AbstractLearnerNetworker &other)  = delete;
  bool               operator==(const AbstractLearnerNetworker &other) = delete;
  bool               operator<(const AbstractLearnerNetworker &other)  = delete;
};

}  // namespace dmlf
}  // namespace fetch
