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

#include <utility>

#include "logging/logging.hpp"
#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-core/agents/Agents.hpp"

class InitialHandshakeTaskFactory : public IOefTaskFactory<OefAgentEndpoint>
{
public:
  static constexpr char const *LOGGING_NAME = "InitialHandshakeTaskFactory";

  InitialHandshakeTaskFactory(std::string core_key, std::shared_ptr<OefAgentEndpoint> the_endpoint,
                              std::shared_ptr<OutboundConversations> outbounds,
                              std::shared_ptr<Agents>                agents)
    : IOefTaskFactory(std::move(the_endpoint), std::move(outbounds))
    , agents_{std::move(agents)}
    , public_key_{""}
    , core_key_{std::move(core_key)}
  {
    state = WAITING_FOR_Agent_Server_ID;
  }
  ~InitialHandshakeTaskFactory() override = default;

  void ProcessMessage(ConstCharArrayBuffer &data) override;
  // Process the message, throw exceptions if they're bad.

  void EndpointClosed() override
  {}

protected:
private:
  enum
  {
    WAITING_FOR_Agent_Server_ID,
    WAITING_FOR_Agent_Server_Answer,
  } state;

  InitialHandshakeTaskFactory(const InitialHandshakeTaskFactory &other) = delete;
  InitialHandshakeTaskFactory &operator=(const InitialHandshakeTaskFactory &other)  = delete;
  bool                         operator==(const InitialHandshakeTaskFactory &other) = delete;
  bool                         operator<(const InitialHandshakeTaskFactory &other)  = delete;

private:
  std::shared_ptr<Agents> agents_;
  std::string             public_key_;
  std::string             core_key_;
};
