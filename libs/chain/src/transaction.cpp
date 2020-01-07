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
#include "chain/transaction_serializer.hpp"
#include "chain/transaction_validity_period.hpp"
#include "crypto/verifier.hpp"

#include <algorithm>
#include <cassert>

namespace fetch {
namespace chain {

/**
 * Compute the total amount being transferred in this transaction
 *
 * @return The total amount being transferred
 */
uint64_t Transaction::GetTotalTransferAmount() const
{
  uint64_t amount{0};

  for (auto const &transfer : transfers_)
  {
    amount += transfer.amount;
  }

  return amount;
}

/**
 * Verify the contents of the transaction
 *
 * @return
 */
bool Transaction::Verify()
{
  if (!verification_completed_)
  {
    // clear the verified flag
    verified_ = false;

    // generate the payload for this transaction
    ConstByteArray payload = TransactionSerializer::SerializePayload(*this);

    // ensure that there are some signatories (otherwise it is invalid)
    if (!signatories_.empty())
    {
      // loop through all the signatories
      bool all_verified{true};
      for (auto const &signatory : signatories_)
      {
        // verify the signature
        if (!crypto::Verifier::Verify(signatory.identity, payload, signatory.signature))
        {
          // exit as soon as the first non valid signature is detected
          all_verified = false;
          break;
        }

        // ensure is well formed
        assert(!signatory.address.address().empty());
      }

      // only valid when there are more that 1 signature present and that they matched the computed
      // payload.
      verified_ = all_verified;
    }

    // signal that the verification has been completed
    verification_completed_ = true;
  }

  return verified_;
}

bool Transaction::IsSignedByFromAddress() const
{
  auto const it = std::find_if(
      signatories_.begin(), signatories_.end(),
      [this](Transaction::Signatory const &signatory) { return signatory.address == from_; });

  return it != signatories_.end();
}

/**
 * Get the digest of the transaction
 *
 * @return The computed transaction
 */
Digest const &Transaction::digest() const
{
  return digest_;
}

/**
 * Get the counter value for the transaction
 *
 * @return The counter value (if one exists)
 */
Transaction::Counter Transaction::counter() const
{
  return counter_;
}

/**
 * Get the sender address for the transaction
 *
 * @return The sender address
 */
Address const &Transaction::from() const
{
  return from_;
}

/**
 * Get the list of transfers for this transaction
 *
 * @return The transfer list
 */
Transaction::Transfers const &Transaction::transfers() const
{
  return transfers_;
}

/**
 * Get the block index from which this transaction becomes valid
 *
 * @return The "valid from" block index
 */
Transaction::BlockIndex Transaction::valid_from() const
{
  return valid_from_;
}

/**
 * Get the block index from which this transaction becomes invalid
 *
 * @return The "valid until" block index
 */
Transaction::BlockIndex Transaction::valid_until() const
{
  return valid_until_;
}

/**
 * Determines the validity of the transaction based on a block index
 *
 * @param block_index The block index being tested
 * @return The validity status for the specified block
 */
Transaction::Validity Transaction::GetValidity(BlockIndex block_index) const
{
  return fetch::chain::GetValidity(*this, block_index);
}

/**
 * Return the charge rate associated with the transaction
 *
 * @return The charge amount
 */
Transaction::TokenAmount Transaction::charge_rate() const
{
  return charge_rate_;
}

/**
 * Get the charge limit associated with the transaction
 *
 * @return The charge limit
 */
Transaction::TokenAmount Transaction::charge_limit() const
{
  return charge_limit_;
}

/**
 * Get the contract mode for this transaction
 *
 * @return The transaction mode
 */
Transaction::ContractMode Transaction::contract_mode() const
{
  return contract_mode_;
}

/**
 * Get the contract address for this smart contract transaction
 *
 * @return The contract address
 */
Address const &Transaction::contract_address() const
{
  return contract_address_;
}

/**
 * Get the chain code identifier for this chain code transaction
 *
 * @return The chain code identifier
 */
Transaction::ConstByteArray const &Transaction::chain_code() const
{
  return chain_code_;
}

/**
 * Get the action being invoked in this transaction
 *
 * @return The action name
 */
Transaction::ConstByteArray const &Transaction::action() const
{
  return action_;
}

/**
 * Get the shard mask associated with this transaction
 *
 * @return The shard mask
 */
BitVector const &Transaction::shard_mask() const
{
  return shard_mask_;
}

/**
 * Get the data payload associated with this transaction
 *
 * @return The data bytes
 */
Transaction::ConstByteArray const &Transaction::data() const
{
  return data_;
}

/**
 * Get the array of signatories associated with this transaction
 *
 * @return The signatories
 */
Transaction::Signatories const &Transaction::signatories() const
{
  return signatories_;
}

/**
 * Check to see if this transaction is verified
 *
 * @return True if the transaction is verified, otherwise false
 */
bool Transaction::IsVerified() const
{
  return verified_;
}

}  // namespace chain
}  // namespace fetch
