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

#include <memory>
#include <utility>

#include "logging/logging.hpp"
#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/proto_comms/TSendProtoTask.hpp"
#include "oef-base/threading/StateMachineTask.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-core/comms/OefAgentEndpoint.hpp"
#include "oef-core/conversations/SearchConversationTypes.hpp"
#include "oef-messages/agent.hpp"

template <typename IN_PROTO, typename OUT_PROTO, typename REQUEST_PROTO, typename IMPL_CLASS>
class SearchConversationTask : public fetch::oef::base::StateMachineTask<IMPL_CLASS>
{
public:
  using StateResult = typename fetch::oef::base::StateMachineTask<IMPL_CLASS>::Result;
  using EntryPoint  = typename fetch::oef::base::StateMachineTask<IMPL_CLASS>::EntryPoint;

  static constexpr char const *LOGGING_NAME = "SearchConversationTask";

  SearchConversationTask(std::string path, std::shared_ptr<IN_PROTO> initiator,
                         std::shared_ptr<OutboundConversations> outbounds,
                         std::shared_ptr<OefAgentEndpoint> endpoint, uint32_t msg_id,
                         std::string core_key, std::string agent_uri, const EntryPoint *entryPoints,
                         IMPL_CLASS *impl_class_ptr)
    : fetch::oef::base::StateMachineTask<IMPL_CLASS>(impl_class_ptr, entryPoints)
    , initiator(std::move(initiator))
    , outbounds(std::move(outbounds))
    , endpoint(std::move(endpoint))
    , msg_id_(msg_id)
    , core_key_{std::move(core_key)}
    , agent_uri_{std::move(agent_uri)}
    , path_{std::move(path)}
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task created.");
  }

  virtual ~SearchConversationTask()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone.");
  }

  StateResult CreateConversation()
  {
    auto                                  this_sp = get_shared();
    std::weak_ptr<fetch::oef::base::Task> this_wp = this_sp;
    FETCH_LOG_INFO(LOGGING_NAME, "Start");
    FETCH_LOG_INFO(LOGGING_NAME, "***PATH: ", path_);
    conversation =
        outbounds->startConversation(Uri("outbound://search:0/" + path_), make_request_proto());

    if (conversation->MakeNotification()
            .Then([this_wp]() {
              auto sp = this_wp.lock();
              if (sp)
              {
                sp->MakeRunnable();
              }
            })
            .Waiting())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Sleeping");
      return SearchConversationTask::StateResult(1, fetch::oef::base::DEFER);
    }
    FETCH_LOG_INFO(LOGGING_NAME, "NOT Sleeping");
    return SearchConversationTask::StateResult(1, fetch::oef::base::COMPLETE);
  }

  virtual StateResult                             HandleResponse()     = 0;
  virtual std::shared_ptr<fetch::oef::base::Task> get_shared()         = 0;
  virtual std::shared_ptr<REQUEST_PROTO>          make_request_proto() = 0;

  using SendFunc =
      std::function<void(std::shared_ptr<OUT_PROTO>, std::shared_ptr<OefAgentEndpoint> endpoint)>;
  SendFunc sendReply;

  void SetDefaultSendReplyFunc(const char *logging_name, const char *log_message)
  {
    sendReply = [logging_name, log_message](std::shared_ptr<OUT_PROTO>        response,
                                            std::shared_ptr<OefAgentEndpoint> e) {
      FETCH_LOG_INFO(logging_name, log_message, response->DebugString());
      auto reply_sender = std::make_shared<
          TSendProtoTask<OefAgentEndpoint, std::shared_ptr<fetch::oef::pb::Server_AgentMessage>>>(
          response, e);
      reply_sender->submit();
    };
  }

protected:
  std::shared_ptr<IN_PROTO>              initiator;
  std::shared_ptr<OutboundConversations> outbounds;
  std::shared_ptr<OutboundConversation>  conversation;
  std::shared_ptr<OefAgentEndpoint>      endpoint;
  uint32_t                               msg_id_;
  std::string                            core_key_;
  std::string                            agent_uri_;
  std::string                            path_;

private:
  SearchConversationTask(const SearchConversationTask &other) = delete;  // { copy(other); }
  SearchConversationTask &operator=(const SearchConversationTask &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const SearchConversationTask &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const SearchConversationTask &other) =
      delete;  // const { return compare(other)==-1; }
};
