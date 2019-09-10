#pragma once

#include "mt-core/conversations/src/cpp/SearchConversationTask.hpp"
#include "protos/src/protos/search_update.pb.h"
#include "protos/src/protos/search_response.pb.h"
#include "protos/src/protos/agent.pb.h"

class OutboundConversations;
class OutboundConversation;
class OefAgentEndpoint;


class SearchAddressUpdateTask
: public SearchConversationTask<
    fetch::oef::pb::Update_Address,
    fetch::oef::pb::UpdateResponse,
    fetch::oef::pb::Update,
    SearchAddressUpdateTask>
{
public:
  using StateResult   = StateMachineTask<SearchAddressUpdateTask>::Result;
  using IN_PROTO      = fetch::oef::pb::Update_Address;
  using OUT_PROTO     = fetch::oef::pb::Server_AgentMessage;
  using REQUEST_PROTO = fetch::oef::pb::Update;

  SearchAddressUpdateTask(std::shared_ptr<IN_PROTO> initiator,
                   std::shared_ptr<OutboundConversations> outbounds,
                   std::shared_ptr<OefAgentEndpoint> endpoint
  );
  virtual ~SearchAddressUpdateTask();

  static constexpr char const *LOGGING_NAME = "SearchAddressUpdateTask";

  std::shared_ptr<Task> get_shared() override
  {
    return shared_from_this();
  }

  StateResult handleResponse(void) override;
  std::shared_ptr<REQUEST_PROTO> make_request_proto() override;

private:
  SearchAddressUpdateTask(const SearchAddressUpdateTask &other) = delete; // { copy(other); }
  SearchAddressUpdateTask &operator=(const SearchAddressUpdateTask &other) = delete; // { copy(other); return *this; }
  bool operator==(const SearchAddressUpdateTask &other) = delete; // const { return compare(other)==0; }
  bool operator<(const SearchAddressUpdateTask &other) = delete; // const { return compare(other)==-1; }
};
