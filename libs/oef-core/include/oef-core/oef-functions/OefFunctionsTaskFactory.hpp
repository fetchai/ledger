#pragma once

#include "logging/logging.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-core/agents/Agents.hpp"

#include "oef-messages/agent.hpp"

namespace google {
namespace protobuf {
class Message;
};
};  // namespace google

class OefFunctionsTaskFactory : public IOefTaskFactory<OefAgentEndpoint>
{
public:
  static constexpr char const *LOGGING_NAME = "OefFunctionsTaskFactory";

  OefFunctionsTaskFactory(const std::string &core_key, std::shared_ptr<Agents> agents,
                          std::string                            agent_public_key,
                          std::shared_ptr<OutboundConversations> outbounds)
    : IOefTaskFactory(outbounds)
    , agents_{std::move(agents)}
    , agent_public_key_{std::move(agent_public_key)}
    , core_key_{core_key}
  {}
  virtual ~OefFunctionsTaskFactory()
  {}

  virtual void processMessage(ConstCharArrayBuffer &data);
  // Process the message, throw exceptions if they're bad.

  virtual void endpointClosed(void);

protected:
private:
  OefFunctionsTaskFactory(const OefFunctionsTaskFactory &other) = delete;
  OefFunctionsTaskFactory &operator=(const OefFunctionsTaskFactory &other)  = delete;
  bool                     operator==(const OefFunctionsTaskFactory &other) = delete;
  bool                     operator<(const OefFunctionsTaskFactory &other)  = delete;

private:
  std::shared_ptr<Agents> agents_;
  std::string             agent_public_key_;
  std::string             core_key_;
};
