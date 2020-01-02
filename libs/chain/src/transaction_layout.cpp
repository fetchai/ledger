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

#include "chain/transaction.hpp"
#include "chain/transaction_layout.hpp"
#include "logging/logging.hpp"
#include "storage/resource_mapper.hpp"

#include <utility>

namespace fetch {
namespace chain {
namespace {

constexpr char const *LOGGING_NAME = "TransactionLayout";

using storage::ResourceAddress;
using byte_array::ConstByteArray;

void UpdateMaskWithTokenAddress(BitVector &shards, Address const &address, uint32_t log2_num_lanes)
{
  // compute the canonical resource for the address
  ConstByteArray const resource = "fetch.token.state." + address.display();

  // compute the resource address
  ResourceAddress const resource_address{resource};

  // update the shard mask
  shards.set(resource_address.lane(log2_num_lanes), 1);
}

}  // namespace

/**
 * Construct a transaction layout from the specified transaction
 *
 * @param tx The input transaction to be summarized
 */
TransactionLayout::TransactionLayout(Transaction const &tx, uint32_t log2_num_lanes)
  : TransactionLayout(tx.digest(), BitVector{1u << log2_num_lanes}, tx.charge_rate(),
                      tx.valid_from(), tx.valid_until())
{
  // in the case where the transaction contains a contract call, ensure that the shard
  // mask is correctly mapped to the current number of lanes
  if (Transaction::ContractMode::NOT_PRESENT != tx.contract_mode())
  {
    if (!tx.shard_mask().RemapTo(mask_))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to remap shard mask");
      return;
    }
  }

  // Every shard mask needs to be updated with the from address so that fees can be removed
  UpdateMaskWithTokenAddress(mask_, tx.from(), log2_num_lanes);

  // since the initial shard mask DOES NOT contain the shard information for the transfers these
  // must now be added.
  for (auto const &transfer : tx.transfers())
  {
    UpdateMaskWithTokenAddress(mask_, transfer.to, log2_num_lanes);
  }
}

/**
 * Construct a transaction layout from its constituent parts
 *
 * @param digest The digest to be set
 * @param mask The mask to be set
 * @param charge_rate The charge rate to be set
 * @param valid_from The block index from which point the transaction is valid
 * @param valid_until The block index from which point the transaction is no longer valid
 */
TransactionLayout::TransactionLayout(Digest digest, BitVector const &mask, TokenAmount charge_rate,
                                     BlockIndex valid_from, BlockIndex valid_until)
  : digest_{std::move(digest)}
  , mask_{mask}
  , charge_rate_{charge_rate}
  , valid_from_{valid_from}
  , valid_until_{valid_until}
{}

/**
 * Get the associated transaction digest
 *
 * @return The transaction digest
 */
TransactionLayout::ConstByteArray const &TransactionLayout::digest() const
{
  return digest_;
}

/**
 * Get the shard mask usage for this transaction
 *
 * @return The shard mask
 */
BitVector const &TransactionLayout::mask() const
{
  return mask_;
}

/**
 * Get the charge (fee) associated with the transaction
 *
 * @return The charge amount
 */
TransactionLayout::TokenAmount TransactionLayout::charge_rate() const
{
  return charge_rate_;
}

/**
 * The block index from which point the transaction is valid
 *
 * @return The block index from when the block is valid
 */
TransactionLayout::BlockIndex TransactionLayout::valid_from() const
{
  return valid_from_;
}

/**
 * The block index until which the transaction is valid
 *
 * @return The block index from which the transaction becomes invalid
 */
TransactionLayout::BlockIndex TransactionLayout::valid_until() const
{
  return valid_until_;
}

/**
 * Determine if the two objects are equal
 *
 * @param other The other layout to compare against
 * @return true if equal, otherwise false
 */
bool TransactionLayout::operator==(TransactionLayout const &other) const
{
  return digest_ == other.digest_;
}

}  // namespace chain
}  // namespace fetch
