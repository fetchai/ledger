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

#include "logging/logging.hpp"
#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-core/agents/Agents.hpp"
//#include "core/crypto/ecdsa.hpp"
#include "oef-core/comms/public_key_utils.hpp"
#include "oef-core/oef-functions/OefFunctionsTaskFactory.hpp"

#include <set>
#include <string>

class InitialSslHandshakeTaskFactory : public IOefTaskFactory<OefAgentEndpoint>
{
public:
  static constexpr char const *LOGGING_NAME = "InitialSslHandshakeTaskFactory";

  InitialSslHandshakeTaskFactory(std::string                            core_key,
                                 std::shared_ptr<OefAgentEndpoint>      the_endpoint,
                                 std::shared_ptr<OutboundConversations> outbounds,
                                 std::shared_ptr<Agents>                agents,
                                 std::shared_ptr<std::set<PublicKey>>   white_list,
                                 bool                                   white_list_enabled)
    : IOefTaskFactory(the_endpoint, outbounds)
    , agents_{std::move(agents)}
    , public_key_{""}
    , core_key_{std::move(core_key)}
    , white_list_{std::move(white_list)}
    , white_list_enabled_{white_list_enabled}
  {}

  virtual ~InitialSslHandshakeTaskFactory()
  {
    FETCH_LOG_WARN(LOGGING_NAME, "~InitialSslHandshakeTaskFactory");
  }

  virtual void ProcessMessage(ConstCharArrayBuffer &data);
  // Process the message, throw exceptions if they're bad.

  virtual void EndpointClosed(void)
  {}

protected:
private:
  InitialSslHandshakeTaskFactory(const InitialSslHandshakeTaskFactory &other) = delete;
  InitialSslHandshakeTaskFactory &operator=(const InitialSslHandshakeTaskFactory &other)  = delete;
  bool                            operator==(const InitialSslHandshakeTaskFactory &other) = delete;
  bool                            operator<(const InitialSslHandshakeTaskFactory &other)  = delete;

private:
  std::shared_ptr<Agents>              agents_;
  std::string                          public_key_;
  std::string                          core_key_;
  std::shared_ptr<EvpPublicKey>        ssl_public_key_;
  std::shared_ptr<std::set<PublicKey>> white_list_;
  bool                                 white_list_enabled_;

  bool load_ssl_pub_keys();
};
