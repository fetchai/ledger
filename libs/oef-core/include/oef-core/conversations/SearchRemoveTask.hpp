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

#include "oef-core/conversations/SearchConversationTask.hpp"
#include "oef-messages/agent.hpp"
#include "oef-messages/search_remove.hpp"
#include "oef-messages/search_response.hpp"

class OutboundConversations;
class OutboundConversation;
class OefAgentEndpoint;

class SearchRemoveTask : public SearchConversationTask<fetch::oef::pb::AgentDescription,
                                                       fetch::oef::pb::Server_AgentMessage,
                                                       fetch::oef::pb::Remove, SearchRemoveTask>
{
public:
  using StateResult   = fetch::oef::base::StateMachineTask<SearchRemoveTask>::Result;
  using IN_PROTO      = fetch::oef::pb::AgentDescription;
  using OUT_PROTO     = fetch::oef::pb::Server_AgentMessage;
  using REQUEST_PROTO = fetch::oef::pb::Remove;
  using PARENT =
      SearchConversationTask<fetch::oef::pb::AgentDescription, fetch::oef::pb::Server_AgentMessage,
                             fetch::oef::pb::Remove, SearchRemoveTask>;

  SearchRemoveTask(std::shared_ptr<IN_PROTO>              initiator,
                   std::shared_ptr<OutboundConversations> outbounds,
                   std::shared_ptr<OefAgentEndpoint> endpoint, uint32_t msg_id,
                   std::string core_key, std::string agent_uri, bool remove_row = false);
  virtual ~SearchRemoveTask();

  static constexpr char const *LOGGING_NAME = "SearchRemoveTask";

  std::shared_ptr<fetch::oef::base::Task> get_shared() override
  {
    return shared_from_this();
  }

  StateResult                    HandleResponse() override;
  std::shared_ptr<REQUEST_PROTO> make_request_proto() override;

private:
  bool remove_row_;

private:
  SearchRemoveTask(const SearchRemoveTask &other) = delete;  // { copy(other); }
  SearchRemoveTask &operator                      =(const SearchRemoveTask &other) =
      delete;                                               // { copy(other); return *this; }
  bool operator==(const SearchRemoveTask &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const SearchRemoveTask &other)  = delete;  // const { return compare(other)==-1; }
};
