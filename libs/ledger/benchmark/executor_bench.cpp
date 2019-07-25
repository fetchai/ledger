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

#include "core/bitvector.hpp"
#include "crypto/ecdsa.hpp"
#include "in_memory_storage.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/chain/transaction_builder.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/executor.hpp"
#include "ledger/identifier.hpp"
#include "ledger/state_sentinel_adapter.hpp"

#include "benchmark/benchmark.h"

#include <memory>

namespace {

using fetch::ledger::Executor;
using fetch::ledger::Transaction;
using fetch::ledger::TransactionBuilder;
using fetch::ledger::Address;
using fetch::ledger::TokenContract;
using fetch::ledger::StateSentinelAdapter;
using fetch::ledger::Identifier;
using fetch::crypto::ECDSASigner;
using fetch::BitVector;

std::shared_ptr<Transaction> CreateSampleTransaction()
{
  ECDSASigner entity1{};
  ECDSASigner entity2{};
  Address     entity1_address{entity1.identity()};
  Address     entity2_address{entity2.identity()};

  return TransactionBuilder()
      .From(entity1_address)
      .Transfer(entity2_address, 200)
      .ValidUntil(1000)
      .ChargeRate(1)
      .ChargeLimit(50)
      .Signer(entity1.identity())
      .Seal()
      .Sign(entity1)
      .Build();
}

void Executor_BasicBenchmark(benchmark::State &state)
{
  auto     storage = std::make_shared<InMemoryStorageUnit>();
  Executor executor{storage, nullptr};

  // create and add the transaction to storage
  auto tx = CreateSampleTransaction();
  storage->AddTransaction(*tx);

  BitVector shards{1};
  shards.SetAllOne();

  // add funds to ensure the transaction passes
  {
    StateSentinelAdapter adapter{*storage, Identifier{"fetch.token"}, shards};

    TokenContract tokens{};

    tokens.Attach(adapter);
    tokens.AddTokens(tx->from(), 500000);
    tokens.Detach();
  }

  for (auto _ : state)
  {
    executor.Execute(tx->digest(), 1, 1, shards);
  }
}

}  // namespace

BENCHMARK(Executor_BasicBenchmark);
