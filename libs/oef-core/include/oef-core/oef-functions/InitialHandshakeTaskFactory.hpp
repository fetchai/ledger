#pragma once

#include "fetch_teams/ledger/logger.hpp"
#include "mt-core/agents/src/cpp/Agents.hpp"
#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"

class InitialHandshakeTaskFactory : public IOefTaskFactory<OefAgentEndpoint>
{
public:
  static constexpr char const *LOGGING_NAME = "InitialHandshakeTaskFactory";

  InitialHandshakeTaskFactory(std::string core_key, std::shared_ptr<OefAgentEndpoint> the_endpoint,
                              std::shared_ptr<OutboundConversations> outbounds,
                              std::shared_ptr<Agents>                agents)
    : IOefTaskFactory(the_endpoint, outbounds)
    , agents_{std::move(agents)}
    , public_key_{""}
    , core_key_{std::move(core_key)}
  {
    state = WAITING_FOR_Agent_Server_ID;
  }
  virtual ~InitialHandshakeTaskFactory()
  {}

  virtual void processMessage(ConstCharArrayBuffer &data);
  // Process the message, throw exceptions if they're bad.

  virtual void endpointClosed(void)
  {}

protected:
private:
  enum
  {
    WAITING_FOR_Agent_Server_ID,
    WAITING_FOR_Agent_Server_Answer,
  } state;

  InitialHandshakeTaskFactory(const InitialHandshakeTaskFactory &other) = delete;
  InitialHandshakeTaskFactory &operator=(const InitialHandshakeTaskFactory &other)  = delete;
  bool                         operator==(const InitialHandshakeTaskFactory &other) = delete;
  bool                         operator<(const InitialHandshakeTaskFactory &other)  = delete;

private:
  std::shared_ptr<Agents> agents_;
  std::string             public_key_;
  std::string             core_key_;
};
