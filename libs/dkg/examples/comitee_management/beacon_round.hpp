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

struct BeaconRoundDetails
{
  using BeaconManager  = dkg::BeaconManager;
  using Identity       = crypto::Identity;
  using ConstByteArray = byte_array::ConstByteArray;
  using SignatureShare = dkg::BeaconManager::SignedMessage;

  BeaconManager                                manager{};
  std::unordered_set<Identity>                 members{};
  uint64_t                                     round{0};
  std::atomic<bool>                            ready{false};
  std::unordered_map<Identity, ConstByteArray> address_of_identity{};

  SignatureShare                               member_share;
  std::unordered_map<Identity, SignatureShare> all_shares;
};

}  // namespace beacon
}  // namespace fetch