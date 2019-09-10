#pragma once

#include "mt-core/conversations/src/cpp/SearchConversationTask.hpp"
#include "protos/src/protos/search_query.pb.h"
#include "protos/src/protos/agent.pb.h"

class OutboundConversations;
class OutboundConversation;
class OefAgentEndpoint;


class SearchQueryTask
: public SearchConversationTask<
    fetch::oef::pb::AgentSearch,
    fetch::oef::pb::Server_AgentMessage,
    fetch::oef::pb::SearchQuery,
    SearchQueryTask>
{
public:
  using StateResult   = StateMachineTask<SearchQueryTask>::Result;
  using IN_PROTO      = fetch::oef::pb::AgentSearch;
  using OUT_PROTO     = fetch::oef::pb::Server_AgentMessage;
  using REQUEST_PROTO = fetch::oef::pb::SearchQuery;

  SearchQueryTask(std::shared_ptr<IN_PROTO> initiator,
                   std::shared_ptr<OutboundConversations> outbounds,
                   std::shared_ptr<OefAgentEndpoint> endpoint,
                   uint32_t msg_id,
                   std::string core_key,
                   std::string agent_uri,
                   uint16_t ttl
  );
  virtual ~SearchQueryTask();

  static constexpr char const *LOGGING_NAME = "SearchUpdateTask";

  std::shared_ptr<Task> get_shared() override
  {
    return shared_from_this();
  }

  StateResult handleResponse(void) override;
  std::shared_ptr<REQUEST_PROTO> make_request_proto() override;

protected:
  uint16_t ttl_;

private:
  SearchQueryTask(const SearchQueryTask &other) = delete; // { copy(other); }
  SearchQueryTask &operator=(const SearchQueryTask &other) = delete; // { copy(other); return *this; }
  bool operator==(const SearchQueryTask &other) = delete; // const { return compare(other)==0; }
  bool operator<(const SearchQueryTask &other) = delete; // const { return compare(other)==-1; }
};
