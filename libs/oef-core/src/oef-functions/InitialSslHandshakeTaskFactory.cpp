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

#include "oef-base/proto_comms/TSendProtoTask.hpp"
#include "oef-core/agents/Agent.hpp"
#include "oef-core/comms/EndpointSSL.hpp"
#include "oef-core/comms/OefAgentEndpoint.hpp"
#include "oef-core/oef-functions/InitialSslHandshakeTaskFactory.hpp"
#include "oef-core/oef-functions/OefFunctionsTaskFactory.hpp"
#include "oef-core/oef-functions/OefHeartbeatTask.hpp"

#include "oef-messages/agent.hpp"

#include <fstream>
#include <string>

// openssl utils
extern std::string RSA_Modulus_from_PEM_f(std::string file_path);
extern std::string RSA_Modulus_short_format(std::string modulus);

void InitialSslHandshakeTaskFactory::ProcessMessage(ConstCharArrayBuffer &data)
{
  // TOFIX protocol msg only, its content will not be used
  fetch::oef::pb::Agent_Server_Answer hi_pb;
  ConstCharArrayBuffer                buff(data);
  IOefTaskFactory::read(hi_pb, buff);  //
  GetEndpoint()->capabilities.will_heartbeat = hi_pb.capability_bits().will_heartbeat();

  // get ssl key
  ssl_public_key_ =
      std::dynamic_pointer_cast<EndpointSSL<std::shared_ptr<google::protobuf::Message>>>(
          this->GetEndpoint()->GetEndpoint()->GetEndpoint())
          ->get_peer_ssl_key();
  public_key_ = RSA_Modulus_short(*ssl_public_key_);

  auto iter = white_list_->find(ssl_public_key_->to_string());
  if (iter != white_list_->end() || !white_list_enabled_)
  {
    // send success
    auto connected_pb = std::make_shared<fetch::oef::pb::Server_Connected>();
    connected_pb->set_status(true);

    FETCH_LOG_INFO(LOGGING_NAME, "Agent ", public_key_,
                   " ssl authenticated and white listed ( or white list disabled = ",
                   white_list_enabled_, "), moving to OefFunctions...");
    auto senderTask = std::make_shared<
        TSendProtoTask<OefAgentEndpoint, std::shared_ptr<fetch::oef::pb::Server_Connected>>>(
        connected_pb, GetEndpoint());
    senderTask->submit();

    agents_->add(public_key_, GetEndpoint());

    GetEndpoint()->karma.upgrade("", public_key_);
    GetEndpoint()->karma.perform("login");

    if (GetEndpoint()->capabilities.will_heartbeat)
    {
      auto heartbeat = std::make_shared<OefHeartbeatTask>(GetEndpoint());
      heartbeat->submit();
    }

    GetEndpoint()->SetState("loggedin", true);
    GetEndpoint()->SetState("ssl", true);

    successor(
        std::make_shared<OefFunctionsTaskFactory>(core_key_, agents_, public_key_, outbounds));
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Agent ", public_key_,
                   " ssl authenticated and NOT white listed. Disconnecting ... ",
                   white_list_->size());

    // send failure
    auto connected_pb = std::make_shared<fetch::oef::pb::Server_Connected>();
    connected_pb->set_status(false);

    auto senderTask = std::make_shared<
        TSendProtoTask<OefAgentEndpoint, std::shared_ptr<fetch::oef::pb::Server_Connected>>>(
        connected_pb, GetEndpoint());
    senderTask->submit();
  }
}
