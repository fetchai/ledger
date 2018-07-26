#pragma once
#include "core/byte_array/byte_array.hpp"
#include "crypto/fnv.hpp"
#include "crypto/identity.hpp"
#include "crypto/prover.hpp"
#include "crypto/verifier.hpp"
#include <atomic>

namespace fetch {
namespace ledger {

struct LaneConnectivityDetails
{
  LaneConnectivityDetails()
      : is_controller(false), is_peer(false), is_outgoing(false)
  {}

  crypto::Identity  identity;
  std::atomic<bool> is_controller;
  std::atomic<bool> is_peer;
  std::atomic<bool> is_outgoing;
};

}  // namespace ledger
}  // namespace fetch
