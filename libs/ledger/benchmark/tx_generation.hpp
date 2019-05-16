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
#include "ledger/chain/v2/transaction.hpp"
#include "ledger/chain/v2/transaction_builder.hpp"

#include <cstdint>

namespace {

using fetch::crypto::ECDSASigner;
using fetch::ledger::v2::Transaction;
using fetch::ledger::v2::TransactionBuilder;
using fetch::ledger::v2::Address;
using fetch::BitVector;

using TransactionList = std::vector<TransactionBuilder::TransactionPtr>;

inline TransactionList GenerateTransactions(std::size_t count, ECDSASigner &signer)
{
  TransactionList list;
  list.reserve(count);

  for (std::size_t i = 0; i < count; ++i)
  {
    auto tx = TransactionBuilder()
                  .From(Address{signer.identity()})
                  .TargetChainCode("fetch.dummy", BitVector{})
                  .Signer(signer.identity())
                  .Seal()
                  .Sign(signer)
                  .Build();

    list.emplace_back(std::move(tx));
  }

  return list;
}

}  // namespace
