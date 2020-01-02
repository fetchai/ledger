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
#include "chain/transaction_builder.hpp"
#include "core/random/lcg.hpp"
#include "crypto/ecdsa.hpp"
#include "meta/type_traits.hpp"

#include <cstdint>

using fetch::crypto::ECDSASigner;
using fetch::chain::Transaction;
using fetch::chain::TransactionBuilder;
using fetch::byte_array::ByteArray;
using TransactionList = std::vector<TransactionBuilder::TransactionPtr>;

template <typename Word = uint64_t>
fetch::meta::IfIsUnsignedInteger<Word, ByteArray> GenerateRandomArray(
    std::size_t num_of_words, fetch::random::LinearCongruentialGenerator rng)
{
  ByteArray array(sizeof(Word) * num_of_words);
  auto      raw_array = reinterpret_cast<Word *>(array.pointer());

  for (std::size_t i = 0; i < num_of_words; ++i)
  {
    raw_array[i] = rng();
  }
  return array;
}

inline TransactionList GenerateTransactions(std::size_t count, ECDSASigner const &signer,
                                            bool large_packets = false)
{
  using fetch::chain::Address;
  using fetch::BitVector;

  using fetch::random::LinearCongruentialGenerator;

  using Word                                    = uint64_t;
  static constexpr std::size_t TX_SIZE_IN_WORDS = 256ull;

  static fetch::random::LinearCongruentialGenerator rng;

  TransactionList list;
  list.reserve(count);

  for (std::size_t i = 0; i < count; ++i)
  {
    auto tx = TransactionBuilder()
                  .From(Address{signer.identity()})
                  .TargetChainCode("fetch.token", BitVector{})
                  .Data(GenerateRandomArray<Word>(large_packets ? TX_SIZE_IN_WORDS : 1ull, rng))
                  .Signer(signer.identity())
                  .Seal()
                  .Sign(signer)
                  .Build();

    list.emplace_back(std::move(tx));
  }

  return list;
}
