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

#include "oef-search/dap_comms/OutboundDapConversationCreator.hpp"

#include "logging/logging.hpp"
#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/threading/StateMachineTask.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-messages/dap_interface.hpp"

#include "oef-base/conversation/OutboundConversationWorkerTask.hpp"
#include "oef-base/utils/Uri.hpp"

#include <google/protobuf/message.h>

OutboundDapConversationCreator::OutboundDapConversationCreator(const Uri &dap_uri, Core &core,
                                                               const std::string &dap_name)
{
  worker = std::make_shared<OutboundConversationWorkerTask>(core, dap_uri, *this);

  worker->SetGroupId(worker->GetTaskId());

  FETCH_LOG_INFO(LOGGING_NAME, "Creating dap conversation with ", dap_name, " @ ",
                 dap_uri.ToString(), ", group ", worker->GetTaskId());

  worker->submit();
}

OutboundDapConversationCreator::~OutboundDapConversationCreator()
{
  worker.reset();
}

std::shared_ptr<OutboundConversation> OutboundDapConversationCreator::start(
    const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Starting dap conversation with ", target_path.host, ":",
                 target_path.port, "/", target_path.path);
  Lock lock(mutex_);
  auto this_id = ident++;

  std::shared_ptr<OutboundConversation> conv;

  if (target_path.path == "execute")
  {
    conv = std::make_shared<OutboundTypedConversation<IdentifierSequence>>(this_id, target_path,
                                                                           initiator);
  }
  else if (target_path.path == "prepareConstraint" || target_path.path == "prepare")
  {
    conv = std::make_shared<OutboundTypedConversation<ConstructQueryMementoResponse>>(
        this_id, target_path, initiator);
  }
  else if (target_path.path == "calculate")
  {
    conv = std::make_shared<OutboundTypedConversation<ConstructQueryConstraintObjectRequest>>(
        this_id, target_path, initiator);
  }
  else if (target_path.path == "update")
  {
    conv = std::make_shared<OutboundTypedConversation<Successfulness>>(this_id, target_path,
                                                                       initiator);
  }
  else if (target_path.path == "describe")
  {
    conv = std::make_shared<OutboundTypedConversation<DapDescription>>(this_id, target_path,
                                                                       initiator);
  }
  else if (target_path.path == "remove" || target_path.path == "removeRow")
  {
    conv = std::make_shared<OutboundTypedConversation<Successfulness>>(this_id, target_path,
                                                                       initiator);
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Path ", target_path.path, " not supported!");
    throw std::invalid_argument(
        target_path.path + " is not a valid target, to start a OutboundDapConversationCreator!");
  }
  conv->SetId(target_path.ToString());
  ident2conversation_[this_id] = conv;
  worker->post(conv);
  return conv;
}
