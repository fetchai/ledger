#pragma once

#include "base/src/cpp/comms/IOefTaskFactory.hpp"
#include "base/src/cpp/comms/Endianness.hpp"
#include "mt-core/agents/src/cpp/Agents.hpp"
#include "fetch_teams/ledger/logger.hpp"
//#include "fetch_teams/ledger/crypto/ecdsa.hpp"
#include "mt-core/oef-functions/src/cpp/OefFunctionsTaskFactory.hpp"
#include "mt-core/secure/experimental/cpp/public_key_utils.hpp"

#include <string>
#include <set>

class InitialSslHandshakeTaskFactory:public IOefTaskFactory<OefAgentEndpoint>
{
public:
  static constexpr char const *LOGGING_NAME = "InitialSslHandshakeTaskFactory";

  InitialSslHandshakeTaskFactory(std::string core_key, std::shared_ptr<OefAgentEndpoint> the_endpoint,
      std::shared_ptr<OutboundConversations> outbounds, std::shared_ptr<Agents> agents,
      std::shared_ptr<std::set<PublicKey>> white_list, bool white_list_enabled)
    : IOefTaskFactory(the_endpoint, outbounds)
  , agents_{std::move(agents)}
  , public_key_{""}
  , core_key_{std::move(core_key)}
  , white_list_{std::move(white_list)}
  , white_list_enabled_{white_list_enabled}
  {
  }
  
  virtual ~InitialSslHandshakeTaskFactory()
  {
    FETCH_LOG_WARN(LOGGING_NAME, "~InitialSslHandshakeTaskFactory");
  }

  virtual void processMessage(ConstCharArrayBuffer &data);
  // Process the message, throw exceptions if they're bad.

  virtual void endpointClosed(void) {}
protected:
private:

  InitialSslHandshakeTaskFactory(const InitialSslHandshakeTaskFactory &other) = delete;
  InitialSslHandshakeTaskFactory &operator=(const InitialSslHandshakeTaskFactory &other) = delete;
  bool operator==(const InitialSslHandshakeTaskFactory &other) = delete;
  bool operator<(const InitialSslHandshakeTaskFactory &other) = delete;

private:
  std::shared_ptr<Agents> agents_;
  std::string public_key_;
  std::string core_key_;
  std::shared_ptr<EvpPublicKey> ssl_public_key_;
  std::shared_ptr<std::set<PublicKey>> white_list_;
  bool white_list_enabled_;
  
  bool load_ssl_pub_keys();

};
