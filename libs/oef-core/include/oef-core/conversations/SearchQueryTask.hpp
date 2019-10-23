#pragma once

#include "oef-core/conversations/SearchConversationTask.hpp"
#include "oef-messages/agent.hpp"
#include "oef-messages/search_query.hpp"

class OutboundConversations;
class OutboundConversation;
class OefAgentEndpoint;

class SearchQueryTask
  : public SearchConversationTask<fetch::oef::pb::AgentSearch, fetch::oef::pb::Server_AgentMessage,
                                  fetch::oef::pb::SearchQuery, SearchQueryTask>
{
public:
  using StateResult   = StateMachineTask<SearchQueryTask>::Result;
  using IN_PROTO      = fetch::oef::pb::AgentSearch;
  using OUT_PROTO     = fetch::oef::pb::Server_AgentMessage;
  using REQUEST_PROTO = fetch::oef::pb::SearchQuery;

  SearchQueryTask(std::shared_ptr<IN_PROTO>              initiator,
                  std::shared_ptr<OutboundConversations> outbounds,
                  std::shared_ptr<OefAgentEndpoint> endpoint, uint32_t msg_id, std::string core_key,
                  std::string agent_uri, uint16_t ttl, uint64_t random_seed);
  virtual ~SearchQueryTask();

  static constexpr char const *LOGGING_NAME = "SearchQueryTask";

  std::shared_ptr<Task> get_shared() override
  {
    return shared_from_this();
  }

  StateResult                    HandleResponse(void) override;
  std::shared_ptr<REQUEST_PROTO> make_request_proto() override;

protected:
  uint16_t ttl_;
  uint64_t random_seed_;

private:
  SearchQueryTask(const SearchQueryTask &other) = delete;  // { copy(other); }
  SearchQueryTask &operator                     =(const SearchQueryTask &other) =
      delete;                                              // { copy(other); return *this; }
  bool operator==(const SearchQueryTask &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const SearchQueryTask &other)  = delete;  // const { return compare(other)==-1; }
};
