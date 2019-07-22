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

#include "ledger/chain/transaction_builder.hpp"

#include "core/logger.hpp"
#include "core/macros.hpp"
#include "crypto/prover.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serializer.hpp"

#include <algorithm>

static constexpr char const *LOGGING_NAME = "TxBuilder";

namespace fetch {
namespace ledger {

using byte_array::ConstByteArray;

/**
 * Construct the sealed builder from a given transaction pointer
 *
 * @param tx The current partial transaction being built
 */
TransactionBuilder::Sealer::Sealer(TransactionPtr tx)
  : partial_transaction_{std::move(tx)}
{
  // perform some sanity checks on the payload
  if (Transaction::ContractMode::NOT_PRESENT != partial_transaction_->contract_mode_)
  {
    if (partial_transaction_->action_.empty())
    {
      throw std::runtime_error(
          "Malformed transaction, must have an action when contract is specified");
    }
  }

  // ensure there is a from field
  if (partial_transaction_->from_.empty())
  {
    throw std::runtime_error("Malformed transaction, missing 'from' field");
  }

  // serialise the payload of the transaction which can be used for signing
  serialized_payload_ = TransactionSerializer::SerializePayload(*partial_transaction_);
}

/**
 * Sign the transaction with the given prover
 *
 * @param prover The prover to be used to generate a signature
 * @return The sealed builder
 */
TransactionBuilder::Sealer &TransactionBuilder::Sealer::Sign(crypto::Prover const &prover)
{
  using Signatory = Transaction::Signatory;

  auto &signatories = partial_transaction_->signatories_;

  // find the identity to which this prover is associated
  auto it = std::find_if(signatories.begin(), signatories.end(),
                         [&prover](Signatory const &s) { return s.identity == prover.identity(); });

  // ensure that we have found the target signatory
  if (it != signatories.end())
  {
    // sign the serialized payload
    it->signature = prover.Sign(serialized_payload_);
    if (!it->signature.empty())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Signed: ", it->signature.ToHex(),
                      " len: ", it->signature.size());
      FETCH_LOG_DEBUG(LOGGING_NAME, "- Payload: ", serialized_payload_.ToHex());
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to sign transaction payload");
    }
  }

  return *this;
}

/**
 * Finalise and complete the transaction being generated
 *
 * @return The finalised transaction
 */
TransactionBuilder::TransactionPtr TransactionBuilder::Sealer::Build()
{
  using Signatory = Transaction::Signatory;

  crypto::SHA256 hash_function{};
  hash_function.Update(serialized_payload_);

  auto const &signatories = partial_transaction_->signatories();

  // ensure that we have at least one signature
  bool valid{false};
  if (!signatories.empty())
  {
    // ensure that none of the signatories have an empty signature field
    valid =
        std::all_of(signatories.begin(), signatories.end(), [&hash_function](Signatory const &s) {
          bool success{false};
          if (!s.signature.empty())
          {
            hash_function.Update(s.signature);
            success = true;
          }
          return success;
        });
  }

  // if valid, extract the transaction
  TransactionPtr tx{};
  if (valid)
  {
    // generate the final transaction
    partial_transaction_->digest_ = hash_function.Final();

    tx = std::move(partial_transaction_);
  }

  return tx;
}

/**
 * Create the transaction builder
 */
TransactionBuilder::TransactionBuilder()
  : partial_transaction_{std::make_unique<Transaction>()}
{}

/**
 * Set the from address for the transaction
 *
 * @param address The from address
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::From(Address const &address)
{
  partial_transaction_->from_ = address;
  return *this;
}

/**
 * Add a transfer to this transaction
 *
 * @param to The destination address
 * @param amount The amount to be transferred
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::Transfer(Address const &to, TokenAmount amount)
{
  using Transfer = Transaction::Transfer;

  auto &transfers = partial_transaction_->transfers_;

  // determine if the address has already been used
  auto it = std::find_if(transfers.begin(), transfers.end(),
                         [&to](Transfer const &t) { return t.to == to; });

  if (it != transfers.end())
  {
    // in the case where we already have a transfer amount. Combine these into a single transfer
    it->amount += amount;
  }
  else
  {
    transfers.emplace_back(Transfer{to, amount});
  }
  return *this;
}

/**
 * Set the valid from field
 *
 * @param index The block number from which this transaction becomes valid
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::ValidFrom(BlockIndex index)
{
  partial_transaction_->valid_from_ = index;
  return *this;
}

/**
 * Set the valid until field
 *
 * @param index The block number from which this transaction becomes invalid
 * @return THe current builder instance
 */
TransactionBuilder &TransactionBuilder::ValidUntil(BlockIndex index)
{
  partial_transaction_->valid_until_ = index;
  return *this;
}

/**
 * Set the charge (fee) for this transaction
 *
 * @param amount The charge / fee to be associated with this transaction
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::ChargeRate(TokenAmount amount)
{
  partial_transaction_->charge_ = amount;
  return *this;
}

/**
 * Set the maximum charge (fee) for this transaction
 *
 * @param amount The maximum charge
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::ChargeLimit(TokenAmount amount)
{
  partial_transaction_->charge_limit_ = amount;
  return *this;
}

/**
 * Set the target smart contract
 *
 * @param digest The target contract digest
 * @param address The target contract address
 * @param shard_mask The resource shard mask
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::TargetSmartContract(Address const &  digest,
                                                            Address const &  address,
                                                            BitVector const &shard_mask)
{
  partial_transaction_->contract_mode_    = Transaction::ContractMode::PRESENT;
  partial_transaction_->contract_digest_  = digest;
  partial_transaction_->contract_address_ = address;
  partial_transaction_->chain_code_       = byte_array::ConstByteArray{};
  partial_transaction_->shard_mask_       = shard_mask;
  return *this;
}

/**
 * Set the target chain code
 *
 * @param ref The target chain code name
 * @param shard_mask The resource shard mask
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::TargetChainCode(byte_array::ConstByteArray const &ref,
                                                        BitVector const &shard_mask)
{
  partial_transaction_->contract_mode_    = Transaction::ContractMode::CHAIN_CODE;
  partial_transaction_->contract_digest_  = Address{};
  partial_transaction_->contract_address_ = Address{};
  partial_transaction_->chain_code_       = ref;
  partial_transaction_->shard_mask_       = shard_mask;
  return *this;
}

/**
 * Set the target as a synergetic contract
 *
 * @param digest The target digest of the synergetic contract
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::TargetSynergetic(Address const &digest)
{
  partial_transaction_->contract_mode_    = Transaction::ContractMode ::SYNERGETIC;
  partial_transaction_->contract_digest_  = digest;
  partial_transaction_->contract_address_ = Address{};
  partial_transaction_->chain_code_       = byte_array::ConstByteArray{};
  partial_transaction_->shard_mask_       = BitVector{};
  return *this;
}

/**
 * Set the contract / chain code action to be triggered
 *
 * @param action The action to be triggered
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::Action(byte_array::ConstByteArray const &action)
{
  partial_transaction_->action_ = action;
  return *this;
}

/**
 * Set the data for the transaction
 *
 * @param data The data buffer for the transaction
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::Data(byte_array::ConstByteArray const &data)
{
  partial_transaction_->data_ = data;
  return *this;
}

/**
 * Add a signer identity to the transaction
 *
 * This in turn will mean that this identity must sign the transaction in order for the transaction
 * to be well formed.
 *
 * @param identity The identity that will be signining
 * @return The current builder instance
 */
TransactionBuilder &TransactionBuilder::Signer(crypto::Identity const &identity)
{
  using Signatory = Transaction::Signatory;

  auto &signatories = partial_transaction_->signatories_;

  auto it = std::find_if(signatories.begin(), signatories.end(),
                         [&identity](Signatory const &s) { return s.identity == identity; });

  // restrict duplicates being added to the list
  if (it == signatories.end())
  {
    // the identity is unique great success!
    signatories.emplace_back(Signatory{identity, Address{identity}, ConstByteArray{}});
  }

  return *this;
}

/**
 * Seal the transaction builder.
 *
 * This step effectively marks the end of the payload for the transaction. The sealed builder which
 * is returned
 * @return
 */
TransactionBuilder::Sealer TransactionBuilder::Seal()
{
  // create the sealer
  Sealer sealer{std::move(partial_transaction_)};

  return sealer;
}

}  // namespace ledger
}  // namespace fetch
