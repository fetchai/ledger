#pragma once

#include "mt-core/conversations/src/cpp/SearchConversationTask.hpp"
#include "protos/src/protos/search_update.pb.h"
#include "protos/src/protos/agent.pb.h"

class OutboundConversations;
class OutboundConversation;
class OefAgentEndpoint;


class SearchUpdateTask
: public SearchConversationTask<
    fetch::oef::pb::AgentDescription,
    fetch::oef::pb::Server_AgentMessage,
    fetch::oef::pb::Update,
    SearchUpdateTask>
{
public:
  using StateResult   = StateMachineTask<SearchUpdateTask>::Result;
  using IN_PROTO      = fetch::oef::pb::AgentDescription;
  using OUT_PROTO     = fetch::oef::pb::Server_AgentMessage;
  using REQUEST_PROTO = fetch::oef::pb::Update;

  SearchUpdateTask(std::shared_ptr<IN_PROTO> initiator,
                   std::shared_ptr<OutboundConversations> outbounds,
                   std::shared_ptr<OefAgentEndpoint> endpoint,
                   uint32_t msg_id,
                   std::string core_key,
                   std::string agent_uri
  );
  virtual ~SearchUpdateTask();

  static constexpr char const *LOGGING_NAME = "SearchUpdateTask";

  std::shared_ptr<Task> get_shared() override
  {
    return shared_from_this();
  }

  StateResult handleResponse(void) override;
  std::shared_ptr<REQUEST_PROTO> make_request_proto() override;

private:
  SearchUpdateTask(const SearchUpdateTask &other) = delete; // { copy(other); }
  SearchUpdateTask &operator=(const SearchUpdateTask &other) = delete; // { copy(other); return *this; }
  bool operator==(const SearchUpdateTask &other) = delete; // const { return compare(other)==0; }
  bool operator<(const SearchUpdateTask &other) = delete; // const { return compare(other)==-1; }
};
