#pragma once

#include "core/byte_array/encoders.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "crypto/identity.hpp"
#include "crypto/prover.hpp"
#include "crypto/verifier.hpp"

#include <atomic>

namespace fetch {
namespace chain {

struct MainChainDetails
{
  MainChainDetails()
    : is_controller(false)
    , is_peer(false)
    , is_miner(false)
    , is_outgoing(false)
  {
  }

  MainChainDetails &operator=(const MainChainDetails &other)
  {
    identity = other.identity;
    owning_discovery_service_identity = other.owning_discovery_service_identity;

    is_controller = false;
    is_peer = false;
    is_miner = false;
    is_outgoing = false;

    return *this;
  }

  MainChainDetails(const MainChainDetails &other)
    : is_controller(false)
    , is_peer(false)
    , is_miner(false)
    , is_outgoing(false)
  {
    identity = other.identity;
    owning_discovery_service_identity = other.owning_discovery_service_identity;
  }

  void CopyFromRemotePeer(const MainChainDetails &incoming_details)
  {
    is_controller = false;
    is_peer = true;
    is_miner = false;

    identity = incoming_details.identity;
    owning_discovery_service_identity = incoming_details.owning_discovery_service_identity;
  }

  void Sign(crypto::Prover *prov)
  {
    serializers::ByteArrayBuffer buffer;
    // TODO: Count count first
    buffer << identity << owning_discovery_service_identity;
    prov->Sign(buffer.data());
    signature = prov->signature();
  }

  std::string GetOwnerIdentityString()
  {
    return std::string(byte_array::ToBase64(owning_discovery_service_identity.identifier()));
  }

  bool Verify(crypto::Verifier *ver)
  {

    serializers::ByteArrayBuffer buffer;
    // TODO: Count count first
    buffer << identity << owning_discovery_service_identity;
    return ver->Verify(buffer.data(), signature);
  }

  /// Serializable fields
  /// @{
  crypto::Identity      identity;
  crypto::Identity      owning_discovery_service_identity;
  byte_array::ConstByteArray signature;
  /// @}

  std::atomic<bool> is_controller;
  std::atomic<bool> is_peer;
  std::atomic<bool> is_miner;
  std::atomic<bool> is_outgoing; // TODO(TR) Consider if this should be removed.
};

template <typename T>
T &Serialize(T &serializer, MainChainDetails const &data)
{
  serializer << data.identity;
  serializer << data.owning_discovery_service_identity;
  return serializer;
}

template <typename T>
T &Deserialize(T &serializer, MainChainDetails &data)
{
  serializer >> data.identity;
  serializer >> data.owning_discovery_service_identity;
  return serializer;
}

  
}  // namespace chain
}  // namespace fetch
