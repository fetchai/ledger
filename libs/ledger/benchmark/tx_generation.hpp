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

#include "core/random/lcg.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"

#include <cstdint>

namespace {

using fetch::crypto::ECDSASigner;
using fetch::ledger::VerifiedTransaction;

using TransactionList = std::vector<VerifiedTransaction>;

inline TransactionList GenerateTransactions(std::size_t count, bool large_packets,
                                            ECDSASigner *signer = nullptr)
{
  static constexpr std::size_t TX_SIZE      = 2048;
  static constexpr std::size_t TX_WORD_SIZE = TX_SIZE / sizeof(uint64_t);
  static_assert((TX_SIZE % sizeof(uint64_t)) == 0, "The transaction must be a multiple of 64bits");

  using fetch::ledger::UnverifiedTransaction;
  using fetch::ledger::MutableTransaction;
  using fetch::ledger::TxSigningAdapter;
  using fetch::byte_array::ByteArray;
  using fetch::ledger::TxSigningAdapter;
  using fetch::random::LinearCongruentialGenerator;

  LinearCongruentialGenerator rng;

  TransactionList list;
  list.reserve(count);

  for (std::size_t i = 0; i < count; ++i)
  {
    MutableTransaction mtx;
    mtx.set_contract_name("fetch.dummy");

    if (large_packets)
    {
      ByteArray tx_data(TX_SIZE);
      uint64_t *tx_data_raw = reinterpret_cast<uint64_t *>(tx_data.pointer());

      for (std::size_t j = 0; j < TX_WORD_SIZE; ++j)
      {
        *tx_data_raw++ = rng();
      }
    }
    else
    {
      mtx.set_data(std::to_string(i));
    }

    // if a signature has been defined then sign the transactions
    if (signer)
    {
      mtx.Sign(signer->private_key());
    }

    list.emplace_back(VerifiedTransaction::Create(std::move(mtx)));
  }

  return list;
}

}  // namespace
