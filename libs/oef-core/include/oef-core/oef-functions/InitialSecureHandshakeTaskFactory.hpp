#pragma once
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

  virtual void ProcessMessage(ConstCharArrayBuffer &data)
  {
    (void)data;
  }
  // Process the message, throw exceptions if they're bad.

  virtual void EndpointClosed(void)
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
