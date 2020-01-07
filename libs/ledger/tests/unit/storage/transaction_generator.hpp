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
#include "chain/transaction.hpp"
#include "chain/transaction_builder.hpp"
#include "core/bitvector.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/random/lcg.hpp"
#include "crypto/ecdsa.hpp"

class TransactionGenerator
{
public:
  using Transaction        = fetch::chain::Transaction;
  using TransactionBuilder = fetch::chain::TransactionBuilder;
  using TransactionPtr     = TransactionBuilder::TransactionPtr;
  using Txs                = std::vector<TransactionGenerator::TransactionPtr>;

  TransactionPtr operator()()
  {
    return TransactionBuilder{}
        .From(address_)
        .ValidUntil(1000)
        .TargetChainCode("foo.bar.baz", BitVector{})
        .Action("test")
        .Data(GenerateRandomData())
        .Signer(public_key_)
        .Seal()
        .Sign(private_key_)
        .Build();
  }

  Txs GenerateRandomTxs(std::size_t count)
  {
    TransactionGenerator &self = *this;

    Txs txs;
    txs.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
    {
      txs.emplace_back(self());
    }

    return txs;
  }

private:
  using Rng            = fetch::random::LinearCongruentialGenerator;
  using Address        = fetch::chain::Address;
  using ECDSASigner    = fetch::crypto::ECDSASigner;
  using Identity       = fetch::crypto::Identity;
  using BitVector      = fetch::BitVector;
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using ByteArray      = fetch::byte_array::ByteArray;

  ConstByteArray GenerateRandomData()
  {
    static const std::size_t RANDOM_WORDS = 5;
    static const std::size_t RANDOM_BYTES = RANDOM_WORDS * sizeof(Rng::result_type);

    ByteArray buffer{};
    buffer.Resize(RANDOM_BYTES);

    auto *raw = reinterpret_cast<Rng::result_type *>(buffer.pointer());
    for (std::size_t i = 0; i < RANDOM_WORDS; ++i)
    {
      raw[i] = rng_();
    }

    return {buffer};
  }

  ECDSASigner private_key_{};
  Identity    public_key_{private_key_.identity()};
  Address     address_{public_key_};
  Rng         rng_{};
};
