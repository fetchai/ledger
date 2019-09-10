#pragma once

#include "base/src/cpp/threading/TNonBlockingWorkerTask.hpp"

#include "base/src/cpp/comms/ConstCharArrayBuffer.hpp"
#include "base/src/cpp/comms/Endpoint.hpp"

#include "base/src/cpp/proto_comms/ProtoPathMessageSender.hpp"
#include "base/src/cpp/proto_comms/ProtoPathMessageReader.hpp"

#include "base/src/cpp/conversation/OutboundConversation.hpp"
#include "base/src/cpp/conversation/OutboundTypedConversation.hpp"

#include "base/src/cpp/utils/Uri.hpp"
#include "fetch_teams/ledger/logger.hpp"


#define TNONBLOCKINGWORKERTASK_SIZE 5


class OutboundConversationWorkerTask
    : public TNonBlockingWorkerTask<OutboundConversation, TNONBLOCKINGWORKERTASK_SIZE>
{
public:
  using Parent            = TNonBlockingWorkerTask<OutboundConversation, TNONBLOCKINGWORKERTASK_SIZE>;
  using Workload          = Parent::Workload;
  using WorkloadP         = Parent::WorkloadP;
  using WorkloadState     = Parent::WorkloadState;
  using WorkloadProcessed = Parent::WorkloadProcessed;
  using TXType            = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;
  using EndpointType      = ProtoMessageEndpoint<TXType, ProtoPathMessageReader, ProtoPathMessageSender>;
  using ConversationMap   = std::map<unsigned long, std::shared_ptr<OutboundConversation>>;



  static constexpr char const *LOGGING_NAME = "OutboundConversationWorkerTask";

  OutboundConversationWorkerTask(Core &core, const Uri &uri, const ConversationMap& conversationMap)
      : uri(uri)
      , core(core)
      , conversationMap(conversationMap)
  {
  }

  virtual ~OutboundConversationWorkerTask()
  {
  }

  virtual WorkloadProcessed process(WorkloadP workload, WorkloadState state) override;

protected:
  virtual bool connect();

protected:
  std::shared_ptr<EndpointType> ep;

  Uri uri;
  Core &core;
  const ConversationMap& conversationMap;

};

// ------------------------------------------------------------------------------------------

bool OutboundConversationWorkerTask::connect()
{
  try
  {
    std::unordered_map<std::string, std::string> endpointConfig;
    auto ep0 = std::make_shared<Endpoint<TXType>>(core, 1000000, 1000000, endpointConfig);
    ep = std::make_shared<EndpointType>(std::move(ep0));

    ep -> setup(ep);
    ep -> connect(uri, core);

    ep->setOnCompleteHandler([this](bool success, unsigned long id, Uri uri_, ConstCharArrayBuffer buffer){
      FETCH_LOG_INFO(LOGGING_NAME,"complete message ", id);

      auto iter = conversationMap.find(id);

      if (iter != conversationMap.end())
      {
        FETCH_LOG_INFO(LOGGING_NAME,"wakeup!!");
        iter -> second -> handleMessage(buffer);
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME,"complete message not handled");
      }
    });

    ep->setOnPeerErrorHandler([this](unsigned long id, int status_code, const std::string& message) {
      FETCH_LOG_INFO(LOGGING_NAME,"error message ", id);
      auto iter = conversationMap.find(id);

      if (iter != conversationMap.end())
      {
        FETCH_LOG_INFO(LOGGING_NAME,"wakeup!!");
        iter -> second -> handleError(status_code, message);
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME,"complete message not handled");
      }
    });

    ep -> go();
    FETCH_LOG_WARN(LOGGING_NAME, "Connected");
    return true;
  }
  catch(std::exception &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, ex.what());
  }
  return false;
}


OutboundConversationWorkerTask::WorkloadProcessed OutboundConversationWorkerTask::process(WorkloadP workload, WorkloadState state)
{
  FETCH_LOG_WARN(LOGGING_NAME, "process search conversation...");
  if (!ep || !ep -> connected())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "no search conn");
    if (!connect())
    {
      return WorkloadProcessed::NOT_STARTED;
    }
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Send initiator...");
  Uri uri(workload -> uri_);
  uri.port = workload -> ident_;
  auto data = std::make_pair(uri, workload->proto_);
  ep -> send(data);
  FETCH_LOG_INFO(LOGGING_NAME, "Starting search ep send loop...");
  ep -> run_sending();
  FETCH_LOG_INFO(LOGGING_NAME, "done..");
  return WorkloadProcessed::COMPLETE;
}