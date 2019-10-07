#pragma once

#include "oef-base/threading/TNonBlockingWorkerTask.hpp"

#include "oef-base/comms/ConstCharArrayBuffer.hpp"
#include "oef-base/comms/Endpoint.hpp"

#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/proto_comms/ProtoPathMessageReader.hpp"
#include "oef-base/proto_comms/ProtoPathMessageSender.hpp"

#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundTypedConversation.hpp"

#include "logging/logging.hpp"
#include "oef-base/utils/Uri.hpp"

#define TNONBLOCKINGWORKERTASK_SIZE 5
#ifndef CONNECT_FAILURE_LIMIT
#define CONNECT_FAILURE_LIMIT 3
#endif

class OutboundConversationWorkerTask
  : public TNonBlockingWorkerTask<OutboundConversation, TNONBLOCKINGWORKERTASK_SIZE>
{
public:
  using Parent       = TNonBlockingWorkerTask<OutboundConversation, TNONBLOCKINGWORKERTASK_SIZE>;
  using Workload     = Parent::Workload;
  using WorkloadP    = Parent::WorkloadP;
  using TXType       = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;
  using EndpointType = ProtoMessageEndpoint<TXType, ProtoPathMessageReader, ProtoPathMessageSender>;
  using ConversationMap = std::map<unsigned long, std::shared_ptr<OutboundConversation>>;

  static constexpr char const *LOGGING_NAME = "OutboundConversationWorkerTask";

  OutboundConversationWorkerTask(Core &core, const Uri &uri, const ConversationMap &conversationMap)
    : uri(uri)
    , core(core)
    , conversationMap(conversationMap)
    , connect_failures_{0}
  {}

  virtual ~OutboundConversationWorkerTask()
  {}

  virtual WorkloadProcessed process(WorkloadP workload, WorkloadState state) override;

protected:
  virtual bool connect();

  void OnPeerError(unsigned long id, int status_code, const std::string &message);

protected:
  std::shared_ptr<EndpointType> ep;
  uint32_t                      connect_failures_;

  Uri                    uri;
  Core &                 core;
  const ConversationMap &conversationMap;
};
