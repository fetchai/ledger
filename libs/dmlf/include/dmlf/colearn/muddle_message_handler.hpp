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

#include "oef-base/threading/Threadpool.hpp"
#include "oef-base/threading/Taskpool.hpp"

#include "dmlf/networkers/abstract_learner_networker.hpp"
#include "core/byte_array/byte_array.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "network/service/call_context.hpp"
#include "muddle/muddle_interface.hpp"

namespace fetch {
namespace colearn {

class MuddleLearnerNetworkerImpl;

class MuddleMessageHandler
{
public:
  using Bytes = byte_array::ByteArray;
  using CallContext = service::CallContext;
  using MuddlePtr = muddle::MuddlePtr;

  MuddleMessageHandler(MuddleLearnerNetworkerImpl &impl, MuddlePtr muddle);
  virtual ~MuddleMessageHandler();

  MuddleMessageHandler(MuddleMessageHandler const &other) = delete;
  MuddleMessageHandler &operator=(MuddleMessageHandler const &other) = delete;
  bool operator==(MuddleMessageHandler const &other) = delete;
  bool operator<(MuddleMessageHandler const &other) = delete;

  bool supplyUpdate(CallContext const &context, const std::string &type, Bytes const &msg);

protected:
private:
  MuddleLearnerNetworkerImpl &impl_;
  MuddlePtr muddle_;
};

}
}
