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

#include <memory>
#include <string>
#include <utility>

#include "oef-base/threading/Notification.hpp"

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

class OefAgentEndpoint;

class Agent
{
public:
  Agent(std::string key, std::shared_ptr<OefAgentEndpoint> endpoint)
  {
    this->key      = std::move(key);
    this->endpoint = std::move(endpoint);
  }

  ~Agent() = default;

  fetch::oef::base::Notification::NotificationBuilder send(
      std::shared_ptr<google::protobuf::Message> s);

  void run_sending();

  std::string getPublicKey()
  {
    return key;
  }

protected:
private:
  std::string                       key;
  std::shared_ptr<OefAgentEndpoint> endpoint;

  Agent(const Agent &other) = delete;
  Agent &operator=(const Agent &other)  = delete;
  bool   operator==(const Agent &other) = delete;
  bool   operator<(const Agent &other)  = delete;
};
