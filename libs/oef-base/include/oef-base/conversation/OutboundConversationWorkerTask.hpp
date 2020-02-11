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

#include "oef-base/threading/TNonBlockingWorkerTask.hpp"

#include "oef-base/comms/ConstCharArrayBuffer.hpp"
#include "oef-base/comms/Endpoint.hpp"

#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/proto_comms/ProtoPathMessageReader.hpp"
#include "oef-base/proto_comms/ProtoPathMessageSender.hpp"

#include "oef-base/conversation/IOutboundConversationCreator.hpp"
#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundTypedConversation.hpp"

#include "logging/logging.hpp"
#include "oef-base/utils/Uri.hpp"

#define TNONBLOCKINGWORKERTASK_SIZE 5
#ifndef CONNECT_FAILURE_LIMIT
#define CONNECT_FAILURE_LIMIT 3
#endif

class OutboundConversationWorkerTask
  : public fetch::oef::base::TNonBlockingWorkerTask<OutboundConversation,
                                                    TNONBLOCKINGWORKERTASK_SIZE>
{
public:
  using Parent       = TNonBlockingWorkerTask<OutboundConversation, TNONBLOCKINGWORKERTASK_SIZE>;
  using Workload     = Parent::Workload;
  using WorkloadP    = Parent::WorkloadP;
  using TXType       = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;
  using EndpointType = ProtoMessageEndpoint<TXType, ProtoPathMessageReader, ProtoPathMessageSender>;

  static constexpr char const *LOGGING_NAME = "OutboundConversationWorkerTask";

  OutboundConversationWorkerTask(Core &core, const Uri &uri,
                                 const IOutboundConversationCreator &conversation_creator)
    : uri(uri)
    , core(core)
    , conversation_creator_(conversation_creator)
    , connect_failures_{0}
  {}

  ~OutboundConversationWorkerTask() override = default;

  fetch::oef::base::WorkloadProcessed process(WorkloadP                       workload,
                                              fetch::oef::base::WorkloadState state) override;

protected:
  virtual bool connect();

  void OnPeerError(unsigned long id, int status_code, const std::string &message);

protected:
  std::shared_ptr<EndpointType> ep;

  Uri                                 uri;
  Core &                              core;
  const IOutboundConversationCreator &conversation_creator_;

  uint32_t connect_failures_;
};
