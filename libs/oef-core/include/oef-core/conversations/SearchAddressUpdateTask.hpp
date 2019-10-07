#pragma once

#include "oef-core/conversations/SearchConversationTask.hpp"
#include "oef-messages/agent.hpp"
#include "oef-messages/dap_interface.hpp"
#include "oef-messages/search_update.hpp"

class OutboundConversations;
class OutboundConversation;
class OefAgentEndpoint;

class SearchAddressUpdateTask
  : public SearchConversationTask<Address, Successfulness, fetch::oef::pb::Update,
                                  SearchAddressUpdateTask>
{
public:
  using StateResult   = StateMachineTask<SearchAddressUpdateTask>::Result;
  using IN_PROTO      = Address;
  using OUT_PROTO     = fetch::oef::pb::Server_AgentMessage;
  using REQUEST_PROTO = fetch::oef::pb::Update;

  SearchAddressUpdateTask(std::shared_ptr<IN_PROTO>              initiator,
                          std::shared_ptr<OutboundConversations> outbounds,
                          std::shared_ptr<OefAgentEndpoint>      endpoint);
  virtual ~SearchAddressUpdateTask();

  static constexpr char const *LOGGING_NAME = "SearchAddressUpdateTask";

  std::shared_ptr<Task> get_shared() override
  {
    return shared_from_this();
  }

  StateResult                    handleResponse(void) override;
  std::shared_ptr<REQUEST_PROTO> make_request_proto() override;

private:
  SearchAddressUpdateTask(const SearchAddressUpdateTask &other) = delete;  // { copy(other); }
  SearchAddressUpdateTask &operator=(const SearchAddressUpdateTask &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const SearchAddressUpdateTask &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const SearchAddressUpdateTask &other) =
      delete;  // const { return compare(other)==-1; }
};
