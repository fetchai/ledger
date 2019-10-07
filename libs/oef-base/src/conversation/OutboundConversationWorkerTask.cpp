#include "oef-base/conversation/OutboundConversationWorkerTask.hpp"
#include "oef-base/threading/WorkloadState.hpp"

void OutboundConversationWorkerTask::OnPeerError(unsigned long id, int status_code,
                                                 const std::string &message)
{
  FETCH_LOG_INFO(LOGGING_NAME, "error message id=", id);
  auto iter = conversationMap.find(id);

  if (iter != conversationMap.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "wakeup!!");
    iter->second->handleError(status_code, message);
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "complete message not handled");
  }
}

bool OutboundConversationWorkerTask::connect()
{
  try
  {
    std::unordered_map<std::string, std::string> endpointConfig;
    auto ep0 = std::make_shared<Endpoint<TXType>>(core, 1000000, 1000000, endpointConfig);
    ep       = std::make_shared<EndpointType>(std::move(ep0));

    ep->setup(ep);
    ep->connect(uri, core);

    ep->setOnCompleteHandler(
        [this](bool success, unsigned long id, Uri uri_, ConstCharArrayBuffer buffer) {
          FETCH_LOG_INFO(LOGGING_NAME, "complete message ", id);

          auto iter = conversationMap.find(id);

          if (iter != conversationMap.end())
          {
            FETCH_LOG_INFO(LOGGING_NAME, "wakeup!!");
            iter->second->handleMessage(buffer);
          }
          else
          {
            FETCH_LOG_INFO(LOGGING_NAME, "complete message not handled");
          }
        });

    ep->setOnPeerErrorHandler(
        [this](unsigned long id, int status_code, const std::string &message) {
          OnPeerError(id, status_code, message);
        });

    ep->go();
    FETCH_LOG_WARN(LOGGING_NAME, "Connected");
    return true;
  }
  catch (std::exception &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Connection failed: ", ex.what());
    ++connect_failures_;
    ep = nullptr;
  }
  return false;
}

WorkloadProcessed OutboundConversationWorkerTask::process(WorkloadP workload, WorkloadState state)
{
  if (connect_failures_ > CONNECT_FAILURE_LIMIT)
  {
    OnPeerError(workload->GetIdent(), 61, "Connection failure because limit reached!");
    return WorkloadProcessed ::COMPLETE;
  }
  FETCH_LOG_WARN(LOGGING_NAME, "process search conversation...");
  if (!ep || !ep->connected())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "no search conn");
    if (!connect())
    {
      return WorkloadProcessed::NOT_STARTED;
    }
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Send initiator...");
  Uri uri(workload->uri_);
  uri.port  = workload->ident_;
  auto data = std::make_pair(uri, workload->proto_);
  ep->send(data);
  FETCH_LOG_INFO(LOGGING_NAME, "Starting search ep send loop...");
  ep->run_sending();
  FETCH_LOG_INFO(LOGGING_NAME, "done..");
  return WorkloadProcessed::COMPLETE;
}