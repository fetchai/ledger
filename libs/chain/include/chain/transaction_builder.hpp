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

#include "chain/transaction.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <memory>

namespace fetch {
namespace crypto {

class Prover;

}  // namespace crypto

namespace chain {

/**
 * Builder class to construct transactions. This class caches the serial version of the transactions
 * so that it can be used for signing.
 *
 * The builder also restricts the way the user can construct the transaction.
 */
class TransactionBuilder
{
public:
  using TokenAmount    = Transaction::TokenAmount;
  using BlockIndex     = Transaction::BlockIndex;
  using CounterValue   = Transaction::Counter;
  using TransactionPtr = std::shared_ptr<Transaction>;
  using ConstByteArray = byte_array::ConstByteArray;

  /**
   * Special version of builder only allowing signing. This pattern ensures that the user can not
   * sign contents and then try and modify the transaction contents. It is also a nice placeholder
   * generating a serial representation for the transaction.
   */
  struct Sealer
  {
  public:
    // Construction / Destruction
    explicit Sealer(TransactionPtr tx);

    Sealer &       Sign(crypto::Prover const &prover);
    TransactionPtr Build();

  private:
    TransactionPtr partial_transaction_;
    ConstByteArray serialized_payload_;
  };

  // Construction / Destruction
  TransactionBuilder();
  TransactionBuilder(TransactionBuilder const &) = delete;
  TransactionBuilder(TransactionBuilder &&)      = delete;
  ~TransactionBuilder()                          = default;

  /// @name Basic Operations
  /// @{
  TransactionBuilder &From(Address const &address);
  TransactionBuilder &Transfer(Address const &to, TokenAmount amount);
  TransactionBuilder &ValidFrom(BlockIndex index);
  TransactionBuilder &ValidUntil(BlockIndex index);
  TransactionBuilder &ChargeRate(TokenAmount amount);
  TransactionBuilder &ChargeLimit(TokenAmount amount);
  TransactionBuilder &Counter(CounterValue counter);
  /// @}

  /// @name Contract Operations
  /// @{
  TransactionBuilder &TargetSmartContract(Address const &address, BitVector const &shard_mask);
  TransactionBuilder &TargetChainCode(byte_array::ConstByteArray const &ref,
                                      BitVector const &                 shard_mask);
  TransactionBuilder &Action(byte_array::ConstByteArray const &action);
  TransactionBuilder &Data(byte_array::ConstByteArray const &data);
  /// @}

  /// @name Signing Operations
  /// @{
  TransactionBuilder &Signer(crypto::Identity const &identity);
  Sealer              Seal();
  /// @}

  // Operators
  TransactionBuilder &operator=(TransactionBuilder const &) = delete;
  TransactionBuilder &operator=(TransactionBuilder &&) = delete;

private:
  TransactionPtr partial_transaction_;
};

}  // namespace chain
}  // namespace fetch
