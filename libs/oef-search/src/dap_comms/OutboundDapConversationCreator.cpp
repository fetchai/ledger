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

#include "oef-search/dap_comms/OutboundDapConversationCreator.hpp"

#include "logging/logging.hpp"
#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/threading/StateMachineTask.hpp"
#include "oef-base/utils/Uri.hpp"

#include "oef-base/conversation/OutboundConversationWorkerTask.hpp"

#include "oef-messages/fetch_protobuf.hpp"

OutboundDapConversationCreator::OutboundDapConversationCreator(
    size_t thread_group_id, const Uri & /*dap_uri*/, Core & /*core*/,
    std::shared_ptr<OutboundConversations> /*outbounds*/)
{
  // TODO: Does not work
  // worker = std::make_shared<OutboundConversationWorkerTask>(core, dap_uri, outbounds,
  //                                                            ident2conversation);

  worker->setThreadGroupId(thread_group_id);

  worker->submit();
}

OutboundDapConversationCreator::~OutboundDapConversationCreator()
{
  worker.reset();
}

std::shared_ptr<OutboundConversation> OutboundDapConversationCreator::start(
    const Uri & /*target_path*/, std::shared_ptr<google::protobuf::Message> /*initiator*/)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Starting search conversation...");
  auto this_id = ident++;

  std::shared_ptr<OutboundConversation> conv;

  /*if (target_path.path == "/update")
  {
    conv = std::make_shared<OutboundTypedConversation<fetch::oef::pb::UpdateResponse>>(this_id,
  target_path, initiator);
  }
  else if (target_path.path == "/remove")
  {
    conv = std::make_shared<OutboundTypedConversation<fetch::oef::pb::RemoveResponse>>(this_id,
  target_path, initiator);
  }
  else if (target_path.path == "/search")
  {
    conv = std::make_shared<OutboundTypedConversation<fetch::oef::pb::SearchResponse>>(this_id,
  target_path, initiator);
  }
  else
  {
    throw std::invalid_argument(target_path.path + " is not a valid target, to start a
  OutboundDapConversationCreator!");
  }*/

  ident2conversation[this_id] = conv;
  worker->post(conv);
  return conv;
}
