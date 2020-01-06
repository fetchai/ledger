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

#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include "oef-base/utils/OefUri.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-core/conversations/SearchQueryTask.hpp"
#include "oef-messages/dap_interface.hpp"

static Counter tasks_created("mt-core.search.query.tasks_created");
static Counter tasks_resolved("mt-core.search.query.tasks_resolved");
static Counter tasks_replied("mt-core.search.query.tasks_replied");
static Counter tasks_unreplied("mt-core.search.query.tasks_unreplied");
static Counter tasks_succeeded("mt-core.search.query.tasks_succeeded");
static Counter tasks_errored("mt-core.search.query.tasks_errored");

SearchQueryTask::EntryPoint searchQueryTaskEntryPoints[] = {
    &SearchQueryTask::CreateConversation,
    &SearchQueryTask::HandleResponse,
};

SearchQueryTask::SearchQueryTask(std::shared_ptr<SearchQueryTask::IN_PROTO> initiator,
                                 std::shared_ptr<OutboundConversations>     outbounds,
                                 std::shared_ptr<OefAgentEndpoint> endpoint, uint32_t msg_id,
                                 std::string core_key, std::string agent_uri, uint16_t ttl,
                                 uint64_t random_seed)
  : SearchConversationTask("search", std::move(initiator), std::move(outbounds),
                           std::move(endpoint), msg_id, std::move(core_key), std::move(agent_uri),
                           searchQueryTaskEntryPoints, this)
  , ttl_{ttl}
  , random_seed_{random_seed}
{
  tasks_created++;
}

SearchQueryTask::~SearchQueryTask()
{
  tasks_resolved++;
}

SearchQueryTask::StateResult SearchQueryTask::HandleResponse()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Woken ");
  FETCH_LOG_INFO(LOGGING_NAME, "Response.. ", conversation->GetAvailableReplyCount());

  if (conversation->GetAvailableReplyCount() == 0)
  {
    return SearchQueryTask::StateResult(0, fetch::oef::base::ERRORED);
  }

  if (!conversation->success())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Search call returned error...");
    return SearchQueryTask::StateResult(0, fetch::oef::base::ERRORED);
  }

  if (conversation->GetAvailableReplyCount() == 0)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No available reply for search query, waiting more...");
    return SearchQueryTask::StateResult(0, fetch::oef::base::DEFER);
  }

  auto response = std::static_pointer_cast<IdentifierSequence>(conversation->GetReply(0));

  auto answer = std::make_shared<OUT_PROTO>();
  answer->set_answer_id(static_cast<int32_t>(msg_id_));

  if (!response->status().success())
  {
    tasks_errored++;
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Error response from search, code: ", response->status().errorcode(),
                   ", narrative:");
    for (const auto &n : response->status().narrative())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "  ", n);
    }
  }
  else
  {
    std::unordered_map<std::string, std::vector<Identifier *>> results{};
    for (int i = 0; i < response->identifiers_size(); ++i)
    {
      auto *id = response->mutable_identifiers(i);
      results[id->core()].push_back(id);
    }

    if (ttl_ == 1)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Got search response: ", response->DebugString(),
                     ", size: ", response->identifiers_size());

      auto answer_agents = answer->mutable_agents();

      for (auto &item : results)
      {
        for (auto &a : item.second)
        {
          OEFURI::URI uri;
          uri.ParseAgent(a->agent());
          answer_agents->add_agents(uri.AgentPartAsString());
        }
      }
      FETCH_LOG_INFO(LOGGING_NAME, "Sending ", answer_agents->agents().size(), "agents to ",
                     agent_uri_);
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Got wide search response: ", response->DebugString());
      auto agents_wide = answer->mutable_agents_wide();

      int agents_nbr = 0;
      for (auto &item : results)
      {
        auto *aw_item = agents_wide->add_result();

        if (!item.second.empty())
        {
          auto *pt = item.second[0];

          aw_item->set_key(pt->core());

          if (!pt->uri().empty())
          {
            Uri uri(pt->uri());
            aw_item->set_ip(uri.host);
            aw_item->set_port(uri.port);
          }
          aw_item->set_distance(pt->distance());
        }

        agents_nbr += static_cast<int>(item.second.size());
        for (auto &a : item.second)
        {
          auto *aw = aw_item->add_agents();
          aw->set_key(a->agent());
          aw->set_score(a->score());
        }
      }
      tasks_succeeded++;
      FETCH_LOG_INFO(LOGGING_NAME, "Sending ", agents_nbr, "agents to ", agent_uri_);
    }
  }

  if (sendReply)
  {
    tasks_replied++;
    sendReply(answer, endpoint);
  }
  else
  {
    tasks_unreplied++;
    FETCH_LOG_WARN(LOGGING_NAME, "No sendReply!!");
  }

  FETCH_LOG_INFO(LOGGING_NAME, "COMPLETE");

  return SearchQueryTask::StateResult(0, fetch::oef::base::COMPLETE);
}

std::shared_ptr<SearchQueryTask::REQUEST_PROTO> SearchQueryTask::make_request_proto()
{
  auto search_query = std::make_shared<fetch::oef::pb::SearchQuery>();
  search_query->set_source_key(core_key_);
  if (initiator->has_query_v2())
  {
    search_query->mutable_query_v2()->CopyFrom(initiator->query_v2());
  }
  else
  {
    search_query->mutable_model()->CopyFrom(initiator->query());
  }
  search_query->set_ttl(ttl_);
  search_query->set_id(random_seed_);
  FETCH_LOG_INFO(LOGGING_NAME, "Sending query to search: ", search_query->DebugString());
  return search_query;
}
