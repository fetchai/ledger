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

#include "oef-base/threading/Task.hpp"

template <class ENDPOINT, class DATA_TYPE>
class TSendProtoTask : public fetch::oef::base::Task
{
public:
  TSendProtoTask(DATA_TYPE pb, std::shared_ptr<ENDPOINT> endpoint)
    : fetch::oef::base::Task()
    , endpoint{std::move(endpoint)}
    , pb{std::move(pb)}
  {}

  ~TSendProtoTask() override                  = default;
  TSendProtoTask(const TSendProtoTask &other) = delete;
  TSendProtoTask &operator=(const TSendProtoTask &other) = delete;

  bool operator==(const TSendProtoTask &other) = delete;
  bool operator<(const TSendProtoTask &other)  = delete;

  bool IsRunnable() const override
  {
    return true;
  }

  fetch::oef::base::ExitState run() override
  {
    // TODO(kll): it's possible there's a race hazard here. Need to think about this.
    if (endpoint->send(pb).Then([this]() { this->MakeRunnable(); }).Waiting())
    {
      return fetch::oef::base::ExitState::DEFER;
    }

    endpoint->run_sending();
    pb = DATA_TYPE{};
    return fetch::oef::base::ExitState::COMPLETE;
  }

private:
  std::shared_ptr<ENDPOINT> endpoint;
  DATA_TYPE                 pb;
};

// namespace std { template<> void swap(TSendProto& lhs, TSendProto& rhs) { lhs.swap(rhs); } }
// std::ostream& operator<<(std::ostream& os, const TSendProto &output) {}
