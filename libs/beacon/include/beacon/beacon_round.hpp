#pragma once
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/prover.hpp"
#include "dkg/beacon_manager.hpp"

#include <atomic>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace beacon {

struct Aeon
{
  using Identity = crypto::Identity;

  std::unordered_set<Identity> members{};
  uint64_t                     round_start{0};
  uint64_t                     round_end{0};
};

struct BeaconRoundDetails
{
  using BeaconManager  = dkg::BeaconManager;
  using ConstByteArray = byte_array::ConstByteArray;
  using SignatureShare = dkg::BeaconManager::SignedMessage;

  BeaconManager  manager{};
  SignatureShare member_share;

  Aeon aeon;

  std::atomic<bool> ready{false};
  std::atomic<bool> observe_only{false};
};

}  // namespace beacon
}  // namespace fetch