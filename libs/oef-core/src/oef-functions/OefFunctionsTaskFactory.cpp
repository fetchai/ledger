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

#include "oef-core/oef-functions/OefFunctionsTaskFactory.hpp"

#include "logging/logging.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include "oef-base/proto_comms/TSendProtoTask.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-core/conversations/SearchConversationTask.hpp"
#include "oef-core/conversations/SearchConversationTypes.hpp"
#include "oef-core/conversations/SearchQueryTask.hpp"
#include "oef-core/conversations/SearchRemoveTask.hpp"
#include "oef-core/conversations/SearchUpdateTask.hpp"
#include "oef-core/karma/XDisconnect.hpp"
#include "oef-core/karma/XError.hpp"
#include "oef-core/karma/XKarma.hpp"
#include "oef-core/tasks/AgentToAgentMessageTask.hpp"

#include "oef-messages/agent.hpp"
#include "oef-messages/search_query.hpp"
#include "oef-messages/search_remove.hpp"
#include "oef-messages/search_response.hpp"
#include "oef-messages/search_update.hpp"

#include <random>
#include <stdexcept>

static Counter endpoint_closed("mt-core.oef_functions_endpoint_closed");

void OefFunctionsTaskFactory::EndpointClosed()
{
  FETCH_LOG_WARN(LOGGING_NAME, "Endpoint closed for agent: ", agent_public_key_,
                 ". Sending removeRow to search...");
  agents_->remove(agent_public_key_);

  std::random_device                      r;
  std::default_random_engine              e1(r());
  std::uniform_int_distribution<uint32_t> uniform_dist(1000000, 1000000000);

  endpoint_closed++;
  auto convTask = std::make_shared<SearchRemoveTask>(
      nullptr, outbounds, GetEndpoint(), uniform_dist(e1), core_key_, agent_public_key_, true);
  convTask->submit();
  convTask->SetGroupId(0);
}

void OefFunctionsTaskFactory::ProcessMessage(ConstCharArrayBuffer &data)
{
  auto envelope = fetch::oef::pb::Envelope();
  IOefTaskFactory::read(envelope, data, data.size - data.current);

  auto    payload_case = envelope.payload_case();
  int32_t msg_id       = envelope.msg_id();

  OEFURI::URI uri;
  uri.parse(envelope.agent_uri());
  uri.AgentKey = agent_public_key_;

  fetch::oef::pb::Server_AgentMessage_OEFError_Operation operation_code =
      fetch::oef::pb::Server_AgentMessage_OEFError_Operation_OTHER;

  // in case we have to report an error, get the OEFError::Operation
  // code because, sigh, it's different...

  try
  {

    switch (payload_case)
    {
    default:
      GetEndpoint()->karma.perform("oef.bad.unknown-message");
      throw XError("unknown message");
      break;

    case fetch::oef::pb::Envelope::kPong:
    {
      GetEndpoint()->heartbeat_recvd();
      break;
    }

    case fetch::oef::pb::Envelope::kSendMessage:
    {
      operation_code = fetch::oef::pb::Server_AgentMessage_OEFError_Operation_SEND_MESSAGE;
      FETCH_LOG_INFO(LOGGING_NAME, "kSendMessage");
      GetEndpoint()->karma.perform("oef.kSendMessage");
      std::shared_ptr<fetch::oef::pb::Agent_Message> msg_ptr(envelope.release_send_message());
      FETCH_LOG_INFO(LOGGING_NAME, "Got agent message: ", msg_ptr->DebugString());
      auto senderTask = std::make_shared<AgentToAgentMessageTask<fetch::oef::pb::Agent_Message>>(
          agents_->find(agent_public_key_), msg_id, msg_ptr, agents_);
      senderTask->submit();
      break;
    }

    case fetch::oef::pb::Envelope::kRegisterService:
    {
      operation_code = fetch::oef::pb::Server_AgentMessage_OEFError_Operation_REGISTER_SERVICE;
      GetEndpoint()->karma.perform("oef.kRegisterService");
      FETCH_LOG_INFO(LOGGING_NAME, "kRegisterService", envelope.register_service().DebugString());
      auto convTask = std::make_shared<SearchUpdateTask>(
          std::shared_ptr<fetch::oef::pb::AgentDescription>(envelope.release_register_service()),
          outbounds, GetEndpoint(), envelope.msg_id(), core_key_, uri.AgentPartAsString());
      convTask->SetDefaultSendReplyFunc(LOGGING_NAME, "kRegisterService REPLY ");
      convTask->submit();
      break;
    }
    case fetch::oef::pb::Envelope::kUnregisterService:
    {
      operation_code = fetch::oef::pb::Server_AgentMessage_OEFError_Operation_UNREGISTER_SERVICE;
      GetEndpoint()->karma.perform("oef.kUnregisterService");
      FETCH_LOG_INFO(LOGGING_NAME, "kUnregisterService",
                     envelope.unregister_service().DebugString());
      auto convTask = std::make_shared<SearchRemoveTask>(
          std::shared_ptr<fetch::oef::pb::AgentDescription>(envelope.release_unregister_service()),
          outbounds, GetEndpoint(), envelope.msg_id(), core_key_, uri.AgentPartAsString());
      convTask->SetDefaultSendReplyFunc(LOGGING_NAME, "kUnregisterService REPLY ");
      convTask->submit();
      break;
    }
    case fetch::oef::pb::Envelope::kSearchAgents:
    {
      operation_code = fetch::oef::pb::Server_AgentMessage_OEFError_Operation_SEARCH_AGENTS;
      GetEndpoint()->karma.perform("oef.kSearchAgents");
      FETCH_LOG_INFO(LOGGING_NAME, "kSearchAgents", envelope.search_agents().DebugString());
      auto convTask = std::make_shared<SearchQueryTask>(
          std::shared_ptr<fetch::oef::pb::AgentSearch>(envelope.release_search_agents()), outbounds,
          GetEndpoint(), envelope.msg_id(), core_key_, uri.ToString(), 1,
          query_id_distribution_(random_engine_));
      convTask->SetDefaultSendReplyFunc(LOGGING_NAME, "kSearchAgents ");
      convTask->submit();
      break;
    }
    case fetch::oef::pb::Envelope::kSearchServices:
    {
      operation_code = fetch::oef::pb::Server_AgentMessage_OEFError_Operation_SEARCH_SERVICES;
      GetEndpoint()->karma.perform("oef.kSearchServices");
      FETCH_LOG_INFO(LOGGING_NAME, "kSearchServices", envelope.search_services().DebugString());
      auto convTask = std::make_shared<SearchQueryTask>(
          std::shared_ptr<fetch::oef::pb::AgentSearch>(envelope.release_search_services()),
          outbounds, GetEndpoint(), envelope.msg_id(), core_key_, uri.ToString(), 1,
          query_id_distribution_(random_engine_));
      convTask->SetDefaultSendReplyFunc(LOGGING_NAME, "kSearchServices ");
      convTask->submit();
      break;
    }
    case fetch::oef::pb::Envelope::kSearchServicesWide:
    {
      operation_code = fetch::oef::pb::Server_AgentMessage_OEFError_Operation_SEARCH_SERVICES_WIDE;
      GetEndpoint()->karma.perform("oef.kSearchServicesWide");
      FETCH_LOG_INFO(LOGGING_NAME, "kSearchServicesWide",
                     envelope.search_services_wide().DebugString());
      auto convTask = std::make_shared<SearchQueryTask>(
          std::shared_ptr<fetch::oef::pb::AgentSearch>(envelope.release_search_services_wide()),
          outbounds, GetEndpoint(), envelope.msg_id(), core_key_, uri.ToString(), 4,
          query_id_distribution_(random_engine_));
      convTask->SetDefaultSendReplyFunc(LOGGING_NAME, "kSearchServicesWide ");
      convTask->submit();
      break;
    }
    case fetch::oef::pb::Envelope::PAYLOAD_NOT_SET:
      operation_code = fetch::oef::pb::Server_AgentMessage_OEFError_Operation_OTHER;
      GetEndpoint()->karma.perform("oef.bad.nopayload");
      FETCH_LOG_ERROR(LOGGING_NAME, "Cannot process payload ", payload_case, " from ",
                      agent_public_key_);
      throw XError("payload not set");
      break;
    }
  }
  catch (XError const &x)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "XERROR! ", x.what());

    auto error_response = std::make_shared<fetch::oef::pb::Server_AgentMessage>();
    error_response->set_answer_id(msg_id);
    int failure = operation_code;
    error_response->mutable_oef_error()->set_operation(
        fetch::oef::pb::Server_AgentMessage_OEFError_Operation(failure));

    error_response->mutable_oef_error()->set_cause("ERROR");
    error_response->mutable_oef_error()->set_detail(x.what());

    auto senderTask = std::make_shared<
        TSendProtoTask<OefAgentEndpoint, std::shared_ptr<fetch::oef::pb::Server_AgentMessage>>>(
        error_response, GetEndpoint());
    senderTask->submit();
  }
  catch (XKarma const &x)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "XKARMA! ", x.what());

    auto error_response = std::make_shared<fetch::oef::pb::Server_AgentMessage>();
    error_response->set_answer_id(msg_id);
    int failure = operation_code;
    error_response->mutable_oef_error()->set_operation(
        fetch::oef::pb::Server_AgentMessage_OEFError_Operation(failure));

    error_response->mutable_oef_error()->set_cause("KARMA");
    error_response->mutable_oef_error()->set_detail(x.what());

    auto senderTask = std::make_shared<
        TSendProtoTask<OefAgentEndpoint, std::shared_ptr<fetch::oef::pb::Server_AgentMessage>>>(
        error_response, GetEndpoint());
    senderTask->submit();
  }
  catch (XDisconnect const &x)
  {
    FETCH_LOG_INFO(LOGGING_NAME,
                   "XDISCONNECT was thrown while processing OEF operation: ", x.what());
    throw;
  }
}
