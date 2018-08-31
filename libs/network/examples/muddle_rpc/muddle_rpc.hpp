#pragma once

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "crypto/ecdsa.hpp"
#include "network/management/network_manager.hpp"
#include "network/service/protocol.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

#include <iostream>
#include <memory>

static constexpr char const *SERVER_PUBLIC_KEY = "JW0+xMThgoNlD7i8V8Tq65N4FDs7ylTXGkPBS20yNdGkmdpbk6xeUqm4yCQq9ONxR8i+/0xb9AlwRy3UtMQ/6g==";
static constexpr char const *SERVER_PRIVATE_KEY = "kvm7uuP+DE+6d9IVDUwrAqOlEUxRD6iRl3PuLs+9EJc=";
static constexpr char const *CLIENT_PUBLIC_KEY = "o/L5zCjbiN2Ux8yt0KKLdPBxbEepOKU5tlDiaZNy3ot1GAp2DRc21xfZcOrFsXE0Zfr5l8Dy8RY2GqohoHxclQ==";
static constexpr char const *CLIENT_PRIVATE_KEY = "ultGhVjHMgWKOmpVoB/5oHQ1+gze6RhhWfSfU8PwgMo=";

using Prover = fetch::crypto::Prover;
using ProverPtr = std::unique_ptr<Prover>;

static ProverPtr CreateKey(char const *key)
{
  using Signer = fetch::crypto::ECDSASigner;
  using SignerPtr = std::unique_ptr<Signer>;

  SignerPtr signer = std::make_unique<Signer>();
  signer->Load(fetch::byte_array::FromBase64(key));

  std::cout << "Public Key: " << fetch::byte_array::ToBase64(signer->public_key()) << std::endl;

  return signer;
}

class Sample
{
public:

  static constexpr char const *LOGGING_NAME = "Sample";

  uint64_t Add(uint64_t a, uint64_t b)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Calling Add(", a, ',', b, ")");
    return a + b;
  }
};

class SampleProtocol : public fetch::service::Protocol
{
public:

  SampleProtocol(Sample &sample)
  {
    Expose(1, &sample, &Sample::Add);
  }
};