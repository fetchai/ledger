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

#include "dmlf/networkers/abstract_learner_networker.hpp"
#include "oef-base/threading/Threadpool.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "dmlf/update_interface.hpp"
#include "muddle/muddle_interface.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class MuddleLearnerNetworkerImpl
  : public dmlf::AbstractLearnerNetworker
{
public:
  using TaskP = Taskpool::TaskP;
  using MuddlePtr = muddle::MuddlePtr;
  using UpdateInterfacePtr = dmlf::UpdateInterfacePtr;

  MuddleLearnerNetworkerImpl(MuddlePtr mud);
  ~MuddleLearnerNetworkerImpl() override;

  MuddleLearnerNetworkerImpl(MuddleLearnerNetworkerImpl const &other) = delete;
  MuddleLearnerNetworkerImpl &operator=(MuddleLearnerNetworkerImpl const &other) = delete;
  bool operator==(MuddleLearnerNetworkerImpl const &other) = delete;
  bool operator<(MuddleLearnerNetworkerImpl const &other) = delete;

  void PushUpdate(const UpdateInterfacePtr &/*update*/) override
  {
    // TODO
  }

  std::size_t GetPeerCount() const override
  {
    return 0; // TODO
  }

  virtual void submit(const TaskP&t);

protected:
private:
  std::shared_ptr<Taskpool> taskpool;
  std::shared_ptr<Threadpool> tasks_runners;
  MuddlePtr mud_;

  friend class MuddleMessageHandler;
};

}
}
}
