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

#include "oef-core/conversations/OutboundSearchConversationCreator.hpp"

#include "logging/logging.hpp"
#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/threading/StateMachineTask.hpp"
#include "oef-base/utils/Uri.hpp"

#include "oef-core/conversations/SearchAddressUpdateTask.hpp"

#include "oef-base/conversation/OutboundConversationWorkerTask.hpp"
#include "search_message.pb.h"
#include "search_query.pb.h"
#include "search_remove.pb.h"
#include "search_response.pb.h"
#include "search_transport.pb.h"
#include "search_update.pb.h"

#include "oef-messages/fetch_protobuf.hpp"

std::map<unsigned long, std::shared_ptr<OutboundConversation>> ident2conversation;

// ------------------------------------------------------------------------------------------

class OutboundSearchConversationWorkerTask : public OutboundConversationWorkerTask
{
public:
  using ConversationMap = OutboundConversationWorkerTask::ConversationMap;

  static constexpr char const *LOGGING_NAME = "OutboundSearchConversationWorkerTask";

  OutboundSearchConversationWorkerTask(Core &core, const std::string &core_key, const Uri &core_uri,
                                       const Uri &                            search_uri,
                                       std::shared_ptr<OutboundConversations> outbounds,
                                       const ConversationMap &                ConversationMap)
    : OutboundConversationWorkerTask(core, search_uri, ConversationMap)
    , outbounds_(std::move(outbounds))
    , core_uri(core_uri)
    , core_key(core_key)
  {}

  virtual ~OutboundSearchConversationWorkerTask() = default;

protected:
  std::shared_ptr<OutboundConversations> outbounds_;

  Uri         core_uri;
  std::string core_key;

  void register_address()
  {
    auto address = std::make_shared<fetch::oef::pb::Update_Address>();
    address->set_ip(core_uri.host);
    address->set_port(core_uri.port);
    address->set_key(core_key);
    address->set_signature("Sign");

    auto convTask = std::make_shared<SearchAddressUpdateTask>(address, outbounds_, nullptr);
    convTask->submit();
  }
  virtual bool connect() override
  {
    if (OutboundConversationWorkerTask::connect())
    {
      register_address();
      return true;
    }
    return false;
  }
};

// ------------------------------------------------------------------------------------------

OutboundSearchConversationCreator::OutboundSearchConversationCreator(
    const std::string &core_key, const Uri &core_uri, const Uri &search_uri, Core &core,
    std::shared_ptr<OutboundConversations> outbounds)
{
  // TODO: Not working
  //  worker = std::make_shared<OutboundSearchConversationWorkerTask>(search_uri, outbounds,
  //                                                                  ident2conversation);

  worker->setThreadGroupId(0);

  worker->submit();
}

OutboundSearchConversationCreator::~OutboundSearchConversationCreator()
{
  worker.reset();
}

std::shared_ptr<OutboundConversation> OutboundSearchConversationCreator::start(
    const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Starting search conversation...");
  auto this_id = ident++;

  std::shared_ptr<OutboundConversation> conv;

  if (target_path.path == "/update")
  {
    conv = std::make_shared<OutboundTypedConversation<fetch::oef::pb::UpdateResponse>>(
        this_id, target_path, initiator);
  }
  else if (target_path.path == "/remove")
  {
    conv = std::make_shared<OutboundTypedConversation<fetch::oef::pb::RemoveResponse>>(
        this_id, target_path, initiator);
  }
  else if (target_path.path == "/search")
  {
    conv = std::make_shared<OutboundTypedConversation<fetch::oef::pb::SearchResponse>>(
        this_id, target_path, initiator);
  }
  else
  {
    throw std::invalid_argument(
        target_path.path + " is not a valid target, to start a OutboundSearchConversationCreator!");
  }

  ident2conversation[this_id] = conv;
  worker->post(conv);
  return conv;
}
