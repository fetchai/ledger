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

#include "oef-search/search_comms/OutboundSearchConversationCreator.hpp"

#include "logging/logging.hpp"
#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/threading/StateMachineTask.hpp"
#include "oef-base/utils/Uri.hpp"

#include "oef-base/conversation/OutboundConversationWorkerTask.hpp"
#include "oef-messages/dap_interface.hpp"
#include "oef-messages/search_message.hpp"
#include "oef-messages/search_query.hpp"
#include "oef-messages/search_remove.hpp"
#include "oef-messages/search_transport.hpp"
#include "oef-messages/search_update.hpp"

#include <google/protobuf/message.h>

OutboundSearchConversationCreator::OutboundSearchConversationCreator(const Uri &search_uri,
                                                                     Core &     core)
  : search_uri_(search_uri)
{
  worker = std::make_shared<OutboundConversationWorkerTask>(core, search_uri, *this);

  worker->SetGroupId(worker->GetTaskId());

  FETCH_LOG_INFO(LOGGING_NAME, "Creating search to search conversation creator for ",
                 search_uri.ToString(), ", group ", worker->GetTaskId());

  if (!worker->submit())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Submit failed for conversation with ", search_uri.ToString());
  }
}

OutboundSearchConversationCreator::~OutboundSearchConversationCreator()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Removing search to search conversation creator for ",
                 search_uri_.ToString());
  worker.reset();
}

std::shared_ptr<OutboundConversation> OutboundSearchConversationCreator::start(
    const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Starting search to search conversation with ",
                 search_uri_.ToString(), " ...");
  Lock lock(mutex_);
  auto this_id = ident++;

  std::shared_ptr<OutboundConversation> conv;

  if (target_path.path == "/search")
  {
    conv = std::make_shared<OutboundTypedConversation<IdentifierSequence>>(this_id, target_path,
                                                                           initiator);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Path ", target_path.path,
                   " not supported in search to search comm!");
    throw std::invalid_argument(
        target_path.path + " is not a valid target, to start a OutboundSearchConversationCreator!");
  }

  conv->SetId(search_uri_.ToString());
  ident2conversation_[this_id] = conv;
  worker->post(conv);
  return conv;
}
