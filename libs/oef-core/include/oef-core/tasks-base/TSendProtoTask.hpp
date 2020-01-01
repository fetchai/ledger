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

#include "oef-core/tasks-base/IMtCoreTask.hpp"

class OefAgentEndpoint;

template <class PROTOBUF>
class TSendProtoTask : public IMtCoreTask
{
public:
  using Proto  = PROTOBUF;
  using ProtoP = std::shared_ptr<Proto>;

  TSendProtoTask(ProtoP pb, std::shared_ptr<OefAgentEndpoint> endpoint)
    : IMtCoreTask()
  {
    this->endpoint = endpoint;
    this->pb       = pb;
    this->foo      = pb.get();
  }

  virtual ~TSendProtoTask()
  {}

  virtual bool IsRunnable(void) const
  {
    return true;
  }

  virtual ExitState run(void)
  {
    // TODO(kll): it's possible there's a race hazard here. Need to think about this.
    if (endpoint->send(pb).Then([this]() { this->MakeRunnable(); }).Waiting())
    {
      return ExitState::DEFER;
    }

    endpoint->run_sending();
    pb.reset();
    return ExitState::COMPLETE;
  }

protected:
  std::shared_ptr<OefAgentEndpoint> endpoint;
  google::protobuf::Message *       foo;

private:
  ProtoP pb;

  TSendProtoTask(const TSendProtoTask &other) = delete;
  TSendProtoTask &operator=(const TSendProtoTask &other)  = delete;
  bool            operator==(const TSendProtoTask &other) = delete;
  bool            operator<(const TSendProtoTask &other)  = delete;
};

// namespace std { template<> void swap(TSendProto& lhs, TSendProto& rhs) { lhs.swap(rhs); } }
// std::ostream& operator<<(std::ostream& os, const TSendProto &output) {}
