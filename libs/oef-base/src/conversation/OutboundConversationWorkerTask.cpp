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

#include "oef-base/conversation/OutboundConversationWorkerTask.hpp"
#include "oef-base/threading/WorkloadState.hpp"

void OutboundConversationWorkerTask::OnPeerError(unsigned long id, int status_code,
                                                 const std::string &message)
{
  FETCH_LOG_WARN(LOGGING_NAME, "error message uri=(", uri.ToString(), ") id=", id,
                 " message=", message);
  conversation_creator_.HandleError(id, uri, status_code, message);
}

bool OutboundConversationWorkerTask::connect()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Connecting to ", uri.ToString());
  try
  {
    std::unordered_map<std::string, std::string> endpointConfig;
    auto ep0 = std::make_shared<Endpoint<TXType>>(core, 1000000, 1000000, endpointConfig);
    ep       = std::make_shared<EndpointType>(std::move(ep0));

    ep->setup(ep);
    ep->connect(uri, core);

    ep->SetOnCompleteHandler(
        [this](bool /*success*/, unsigned long id, Uri /*uri*/, ConstCharArrayBuffer buffer) {
          FETCH_LOG_INFO(LOGGING_NAME, "complete message from ", uri.ToString(), " id=", id);
          conversation_creator_.HandleMessage(id, uri, std::move(buffer));
        });

    ep->SetOnPeerErrorHandler(
        [this](unsigned long id, int status_code, const std::string &message) {
          OnPeerError(id, status_code, message);
        });

    ep->go();
    FETCH_LOG_WARN(LOGGING_NAME, "Connected to ", uri.ToString());
    return true;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Connection to ", uri.ToString(), " failed: ", ex.what());
    ++connect_failures_;
    ep = nullptr;
  }
  return false;
}

fetch::oef::base::WorkloadProcessed OutboundConversationWorkerTask::process(
    WorkloadP workload, fetch::oef::base::WorkloadState /*state*/)
{
  if (connect_failures_ > CONNECT_FAILURE_LIMIT)
  {
    OnPeerError(workload->GetIdentifier(), 61,
                "Connection (" + uri.ToString() + ") failure because limit reached! ");
    return fetch::oef::base::WorkloadProcessed ::COMPLETE;
  }
  FETCH_LOG_WARN(LOGGING_NAME, "process search conversation (uri=", uri.ToString(), ")...");
  if (!ep || !ep->connected())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "no search conn with ", uri.ToString());
    if (!connect())
    {
      return fetch::oef::base::WorkloadProcessed::NOT_STARTED;
    }
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Send initiator to ", uri.ToString(), "...");
  Uri u(workload->uri_);
  u.port    = static_cast<uint32_t>(workload->ident_);
  auto data = std::make_pair(u, workload->proto_);
  ep->send(data);
  FETCH_LOG_INFO(LOGGING_NAME, "Starting search ep send loop (uri=", u.ToString(), ")...");
  ep->run_sending();
  FETCH_LOG_INFO(LOGGING_NAME, "done (uri=", u.ToString(), ")..");
  return fetch::oef::base::WorkloadProcessed::COMPLETE;
}
