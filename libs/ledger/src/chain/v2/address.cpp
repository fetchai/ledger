//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "ledger/chain/v2/address.hpp"
#include "core/macros.hpp"
#include "crypto/hash.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"

namespace fetch {
namespace ledger {
namespace v2 {
namespace {

/**
 * Helper function to calculation the address checksum
 *
 * @param raw_address The input raw address
 * @return The calculated checksum for the address
 */
Address::ConstByteArray CalculateChecksum(Address::ConstByteArray const &raw_address)
{
  return crypto::Hash<crypto::SHA256>(raw_address).SubArray(0, 4);
}

}  // namespace

/**
 * Create an address from an identity
 *
 * @param identity The input identity to use
 */
Address::Address(crypto::Identity const &identity)
  : address_(crypto::Hash<crypto::SHA256>(identity.identifier()))
  , display_(address_ + CalculateChecksum(address_))
{}

/**
 * Create an identity from a raw address (that is a fixed array of bytes)
 *
 * @param address The input (raw) address to use.
 */
Address::Address(RawAddress const &address)
  : address_(address.data(), address.size())
  , display_(address_ + CalculateChecksum(address_))
{}

Address::Address(ConstByteArray address)
  : address_(std::move(address))
  , display_(address_ + CalculateChecksum(address_))
{
  if (address_.size() != 32)
  {
    throw std::runtime_error("Incorrect address size");
  }
}

}  // namespace v2
}  // namespace ledger
}  // namespace fetch
