#pragma once
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

#include "chain/address.hpp"
#include "core/bitvector.hpp"
#include "core/digest.hpp"
#include "crypto/identity.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace chain {

/**
 * The transaction class
 */
class Transaction
{
public:
  using Identity       = crypto::Identity;
  using ConstByteArray = byte_array::ConstByteArray;
  using TokenAmount    = uint64_t;
  using BlockIndex     = uint64_t;
  using Counter        = uint64_t;

  constexpr static uint64_t   MAXIMUM_TX_CHARGE_LIMIT    = 10000000000;
  constexpr static BlockIndex MAXIMUM_TX_VALIDITY_PERIOD = 40000;
  constexpr static BlockIndex DEFAULT_TX_VALIDITY_PERIOD = 1000;

  /**
   * Represents a single target and token about. The transaction format allows any number of
   * transfers to be made in the course of a single transaction. This structure outlines one of them
   */
  struct Transfer
  {
    Address     to;        ///< The destination address for fund transfers
    TokenAmount amount{};  ///< The amount of tokens being transferred
  };

  /**
   * A signatory is the combination of an identity (public key) and a corresponding signature. This
   * is the primary mechanism for transaction authorization
   */
  struct Signatory
  {
    Identity       identity;   ///< The identity of the signer (public key)
    Address        address;    ///< The address corresponding to the identity
    ConstByteArray signature;  ///< The signature of the tx payload from the signer
  };

  /**
   * Internal enumeration specifying the contract (if any) referenced by this transaction
   */
  enum class ContractMode
  {
    NOT_PRESENT,  ///< The is no contract present, simple token transfer transaction
    PRESENT,      ///< There is a smart contract reference present
    CHAIN_CODE,   ///< There is a reference to chain code (hard coded smart contracts) present
    SYNERGETIC,   ///< Synergetic transaction
  };

  /**
   * Internal enumeration for validity query responses
   */
  enum class Validity
  {
    PENDING,  ///< The transaction is not currently, but is due to be so shortly
    VALID,    ///< The transaction is valid to be included into a block
    INVALID,  ///< The transaction is invalid and should be dropped
  };

  using Transfers   = std::vector<Transfer>;
  using Signatories = std::vector<Signatory>;

  // Construction / Destruction
  Transaction()                    = default;
  Transaction(Transaction const &) = default;
  Transaction(Transaction &&)      = default;
  ~Transaction()                   = default;

  /// @name Identification
  /// @{
  Digest const &digest() const;
  Counter       counter() const;
  /// @}

  /// @name Transfer Accessors
  /// @{
  Address const &  from() const;
  Transfers const &transfers() const;
  uint64_t         GetTotalTransferAmount() const;
  /// @}

  /// @name Validity Accessors
  /// @{
  BlockIndex valid_from() const;
  BlockIndex valid_until() const;
  Validity   GetValidity(BlockIndex block_index) const;
  /// @}

  /// @name Charge Accessors
  /// @{
  TokenAmount charge_rate() const;
  TokenAmount charge_limit() const;
  /// @}

  /// @name Contract Accessors
  /// @{
  ContractMode          contract_mode() const;
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
  bool IsSignedByFromAddress() const;
  /// @}

  // Operators
  Transaction &operator=(Transaction const &) = default;
  Transaction &operator=(Transaction &&) = default;

private:
  /// @name Payload
  /// @{
  Address        from_{};                                    ///< The sender of the TX
  Transfers      transfers_{};                               ///< The list of the transfers
  BlockIndex     valid_from_{0};                             ///< Min. block number before valid
  BlockIndex     valid_until_{0};                            ///< Max. block number before invalid
  TokenAmount    charge_rate_{0};                            ///< The charge rate for the TX
  TokenAmount    charge_limit_{0};                           ///< The maximum charge to be used
  ContractMode   contract_mode_{ContractMode::NOT_PRESENT};  ///< The payload being contained
  Address        contract_address_{};                        ///< The address of the smart contract
  ConstByteArray chain_code_{};                              ///< The name of the chain code
  BitVector      shard_mask_{};                              ///< Shard mask of addition depends
  ConstByteArray action_{};                                  ///< The name of the action invoked
  ConstByteArray data_{};                                    ///< The payload of the transaction
  Signatories    signatories_{};                             ///< The signatories for this tx
  Counter        counter_{0};
  /// @}

  /// @name Metadata
  /// @{
  Digest digest_{};                       ///< The digest of the transaction
  bool   verification_completed_{false};  ///< Signal that the verification has been done
  bool   verified_{false};                ///< The cached result of the verification
  /// @}

  // There are only two ways to generate a transaction, each from one of the two companion classes:
  friend class TransactionBuilder;
  friend class TransactionSerializer;
};

}  // namespace chain
}  // namespace fetch
