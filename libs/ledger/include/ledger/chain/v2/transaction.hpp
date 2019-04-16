#pragma once
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

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/identity.hpp"
#include "ledger/chain/v2/address.hpp"
#include "miner/optimisation/bitvector.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace ledger {
namespace v2 {

/**
 * The transaction class
 */
class Transaction
{
public:
  using Identity       = crypto::Identity;
  using ConstByteArray = byte_array::ConstByteArray;
  using BitVector      = bitmanip::BitVector;
  using TokenAmount    = uint64_t;
  using BlockIndex     = uint64_t;

  struct Transfer
  {
    Address  to;
    uint64_t amount;
  };

  struct Signatory
  {
    Identity       identity;
    ConstByteArray signature;
  };

  enum class ContractMode
  {
    NOT_PRESENT,
    PRESENT,
    CHAIN_CODE,
  };

  enum class Validity
  {
    PENDING,
    VALID,
    INVALID,
  };

  using Transfers   = std::vector<Transfer>;
  using Signatories = std::vector<Signatory>;

  // Construction / Destruction
  Transaction()                    = default;
  Transaction(Transaction const &) = default;
  Transaction(Transaction &&)      = default;
  ~Transaction()                   = default;

  ConstByteArray const &digest() const;

  /// @name Transfer Accessors
  /// @{
  Address const &  from() const;
  Transfers const &transfers() const;
  /// @}

  /// @name Validity Accessors
  /// @{
  BlockIndex valid_from() const;
  BlockIndex valid_until() const;
  Validity   GetValidity(BlockIndex block_index) const;
  /// @}

  /// @name Charge Accessors
  /// @{
  TokenAmount charge() const;
  TokenAmount charge_limit() const;
  /// @}

  /// @name Contract Accessors
  /// @{
  ContractMode          contract_mode() const;
  Address const &       contract_digest() const;
  Address const &       contract_address() const;
  ConstByteArray const &chain_code() const;
  ConstByteArray const &action() const;
  BitVector const &     shard_mask() const;
  ConstByteArray const &data() const;
  Signatories const &   signatories() const;
  /// @}

  /// @name Validation / Verification
  /// @{
  bool Verify();
  bool IsVerified() const;
  /// @}

  // Operators
  Transaction &operator=(Transaction const &) = default;
  Transaction &operator=(Transaction &&) = default;

private:
  /// @name Valid
  /// @{
  Address        from_{};                                    ///< The sender of the TX
  Transfers      transfers_{};                               ///< The list of the transfers
  BlockIndex     valid_from_{0};                             ///< Min. block number before valid
  BlockIndex     valid_until_{0};                            ///< Max. block number before invalid
  TokenAmount    charge_{0};                                 ///< The charge rate for the TX
  TokenAmount    charge_limit_{0};                           ///< The maximum charge to be used
  ContractMode   contract_mode_{ContractMode::NOT_PRESENT};  ///< The payload being contained
  Address        contract_digest_{};                         ///< The digest of the smart contract
  Address        contract_address_{};                        ///< The address of the smart contract
  ConstByteArray chain_code_{};                              ///< The name of the chain code
  BitVector      shard_mask_{};                              ///< Shard mask of addition depends
  ConstByteArray action_{};                                  ///< The name of the action invoked
  ConstByteArray data_{};                                    ///< The payload of the transaction
  Signatories    signatories_{};                             ///< The signatories for this tx
  /// @}

  /// @name Metadata
  /// @{
  ConstByteArray digest_{};                       ///< The digest of the transaction
  bool           verification_completed_{false};  ///< Signal that the verification has been done
  bool           verified_{false};                ///< The cached result of the verification
  /// @}

  // These are the only two ways to generate a transaction from one of the two companion classes
  friend class TransactionBuilder;
  friend class TransactionSerializer;
};

/**
 * Get the digest of the transaction
 *
 * @return The computed transaction
 */
inline Transaction::ConstByteArray const &Transaction::digest() const
{
  return digest_;
}

/**
 * Get the sender address for the transaction
 *
 * @return The sender address
 */
inline Address const &Transaction::from() const
{
  return from_;
}

/**
 * Get the list of transfers for this transaction
 *
 * @return The transfer list
 */
inline Transaction::Transfers const &Transaction::transfers() const
{
  return transfers_;
}

/**
 * Get the block index from which this transaction becomes valid
 *
 * @return The "valid from" block index
 */
inline Transaction::BlockIndex Transaction::valid_from() const
{
  return valid_from_;
}

/**
 * Get the block index from which this transaction becomes invalid
 *
 * @return The "valid until" block index
 */
inline Transaction::BlockIndex Transaction::valid_until() const
{
  return valid_until_;
}

/**
 * Determines the validity of the transaction based on a block index
 *
 * @param block_index The block index being tested
 * @return The validity status for the specified block
 */
inline Transaction::Validity Transaction::GetValidity(BlockIndex block_index) const
{
  Validity validity{Validity::INVALID};

  if (block_index < valid_until_)
  {
    validity = Validity::VALID;

    if (valid_from_ && (valid_from_ > block_index))
    {
      validity = Validity::PENDING;
    }
  }

  return validity;
}

/**
 * Return the charge associated with the transaction
 *
 * @return The charge amount
 */
inline Transaction::TokenAmount Transaction::charge() const
{
  return charge_;
}

/**
 * Get the charge limit associated with the transaction
 *
 * @return The charge limit
 */
inline Transaction::TokenAmount Transaction::charge_limit() const
{
  return charge_limit_;
}

/**
 * Get the contract mode for this transaction
 *
 * @return The transaction mode
 */
inline Transaction::ContractMode Transaction::contract_mode() const
{
  return contract_mode_;
}

/**
 * Get the contract digest for this smart contract transaction
 *
 * @return The contract digest
 */
inline Address const &Transaction::contract_digest() const
{
  return contract_digest_;
}

/**
 * Get the contract address for this smart contract transaction
 *
 * @return The contract address
 */
inline Address const &Transaction::contract_address() const
{
  return contract_address_;
}

/**
 * Get the chain code identifier for this chain code transaction
 *
 * @return The chain code identifier
 */
inline Transaction::ConstByteArray const &Transaction::chain_code() const
{
  return chain_code_;
}

/**
 * Get the action being invoked in this transaction
 *
 * @return The action name
 */
inline Transaction::ConstByteArray const &Transaction::action() const
{
  return action_;
}

/**
 * Get the shard mask associated with this transaction
 *
 * @return The shard mask
 */
inline Transaction::BitVector const &Transaction::shard_mask() const
{
  return shard_mask_;
}

/**
 * Get the data payload associated with this transaction
 *
 * @return The data bytes
 */
inline Transaction::ConstByteArray const &Transaction::data() const
{
  return data_;
}

/**
 * Get the array of signatories associated with this transaction
 *
 * @return The signatories
 */
inline Transaction::Signatories const &Transaction::signatories() const
{
  return signatories_;
}

/**
 * Check to see if this transaction is verified
 *
 * @return True if the transaction is verified, otherwise false
 */
inline bool Transaction::IsVerified() const
{
  return verified_;
}

}  // namespace v2
}  // namespace ledger
}  // namespace fetch