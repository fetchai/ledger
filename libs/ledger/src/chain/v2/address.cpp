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
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/macros.hpp"
#include "crypto/hash.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"

namespace fetch {
namespace ledger {
namespace v2 {
namespace {

using byte_array::ToBase58;
using byte_array::FromBase58;

/**
 * Helper function to calculation the address checksum
 *
 * @param raw_address The input raw address
 * @return The calculated checksum for the address
 */
Address::ConstByteArray CalculateChecksum(Address::ConstByteArray const &raw_address)
{
  return crypto::Hash<crypto::SHA256>(raw_address).SubArray(0, Address::CHECKSUM_LENGTH);
}

}  // namespace

/**
 * Parse an address from a string of characters (not a series of bytes)
 *
 * @param input The input characters to be decoded
 * @param output The output address to be populated
 * @return true if successful, otherwise false
 */
bool Address::Parse(ConstByteArray const &input, Address &output)
{
  bool success{false};

  // decode the whole buffer
  auto const decoded = FromBase58(input);

  // ensure that the decode completed successfully (failure would result in 0 length byte array)
  // and that the buffer is the correct size for an address
  if (TOTAL_LENGTH == decoded.size())
  {
    // split the decoded into address and checksum
    auto const address  = decoded.SubArray(0, RAW_LENGTH);
    auto const checksum = decoded.SubArray(RAW_LENGTH, CHECKSUM_LENGTH);

    // compute the expected checksum and compare
    if (CalculateChecksum(address) == checksum)
    {
      output.address_ = address;
      output.display_ = input;
      success         = true;
    }
  }

  return success;
}

/**
 * Create an address from an identity
 *
 * @param identity The input identity to use
 */
Address::Address(crypto::Identity const &identity)
  : address_(crypto::Hash<crypto::SHA256>(identity.identifier()))
  , display_(ToBase58(address_ + CalculateChecksum(address_)))
{}

/**
 * Create an address from a raw address (that is a fixed array of bytes)
 *
 * @param address The input (raw) address to use.
 */
Address::Address(RawAddress const &address)
  : address_(address.data(), address.size())
  , display_(ToBase58(address_ + CalculateChecksum(address_)))
{}

/**
 * Create an address from the const byte array of raw bytes
 *
 * @param address The raw address
 */
Address::Address(ConstByteArray address)
  : address_(std::move(address))
  , display_(ToBase58(address_ + CalculateChecksum(address_)))
{
  if (address_.size() != std::size_t{RAW_LENGTH})
  {
    throw std::runtime_error("Incorrect address size");
  }
}

}  // namespace v2
}  // namespace ledger
}  // namespace fetch
