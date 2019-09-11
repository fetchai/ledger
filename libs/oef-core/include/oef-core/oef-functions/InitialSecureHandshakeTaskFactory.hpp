#pragma once

#include "crypto/ecdsa.hpp"
#include "logging/logging.hpp"
#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-core/agents/Agents.hpp"

#include <openssl/ssl.h>

class InitialSecureHandshakeTaskFactory : public IOefTaskFactory<OefAgentEndpoint>
{
public:
  static constexpr char const *LOGGING_NAME = "InitialSecureHandshakeTaskFactory";

  InitialSecureHandshakeTaskFactory(std::string                            core_key,
                                    std::shared_ptr<OefAgentEndpoint>      the_endpoint,
                                    std::shared_ptr<OutboundConversations> outbounds,
                                    std::shared_ptr<Agents>                agents)
    : IOefTaskFactory(the_endpoint, outbounds)
    , agents_{std::move(agents)}
    , public_key_{""}
    , core_key_{std::move(core_key)}
  {
    // just to test openssl linkage
    SSL_load_error_strings();

    // just to test ledger/crypto
    using fetch::byte_array::ToBase64;
    using fetch::crypto::ECDSASigner;

    ECDSASigner signer;
    signer.GenerateKeys();

    std::cout << "Public Key...: " << ToBase64(signer.public_key()) << std::endl;
    std::cout << "Private Key..: " << ToBase64(signer.private_key()) << std::endl;
  }
  virtual ~InitialSecureHandshakeTaskFactory()
  {}

  virtual void processMessage(ConstCharArrayBuffer &data);
  // Process the message, throw exceptions if they're bad.

  virtual void endpointClosed(void)
  {}

protected:
private:
  InitialSecureHandshakeTaskFactory(const InitialSecureHandshakeTaskFactory &other) = delete;
  InitialSecureHandshakeTaskFactory &operator=(const InitialSecureHandshakeTaskFactory &other) =
      delete;
  bool operator==(const InitialSecureHandshakeTaskFactory &other) = delete;
  bool operator<(const InitialSecureHandshakeTaskFactory &other)  = delete;

private:
  std::shared_ptr<Agents> agents_;
  std::string             public_key_;
  std::string             core_key_;
};
