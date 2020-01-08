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

#include "chain/transaction_builder.hpp"
#include "chain/transaction_rpc_serializers.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/random/lcg.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "ledger/storage_unit/transaction_storage_engine.hpp"
#include "ledger/storage_unit/transaction_store.hpp"

#include "benchmark/benchmark.h"

#include "tx_generation.hpp"

#include <vector>

namespace {

using fetch::chain::Transaction;
using fetch::chain::TransactionBuilder;
using fetch::crypto::ECDSASigner;
using fetch::ledger::TransactionStore;
using fetch::ledger::TransactionStorageEngine;

using TransactionList = std::vector<TransactionBuilder::TransactionPtr>;

constexpr uint32_t LANE_ID        = 0;
constexpr uint32_t LOG2_NUM_LANES = 2;

void TxSubmitFixedLarge(benchmark::State &state)
{
  ECDSASigner const signer;
  // create the transaction store
  TransactionStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(50000, signer, true);

  for (auto _ : state)
  {
    for (auto const &tx : transactions)
    {
      tx_store.Add(*tx);
    }
  }
}

void TxSubmitFixedSmall(benchmark::State &state)
{
  ECDSASigner const signer;

  // create the transaction store
  TransactionStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(50000, signer, false);

  for (auto _ : state)
  {
    for (auto const &tx : transactions)
    {
      tx_store.Add(*tx);
    }
  }
}

void TxSubmitSingleLarge(benchmark::State &state)
{
  ECDSASigner const signer;

  // create the transaction store
  TransactionStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(state.max_iterations, signer, true);

  std::size_t tx_index{0};
  for (auto _ : state)
  {
    auto const &tx = transactions.at(tx_index++);
    tx_store.Add(*tx);
  }
}

void TxSubmitSingleSmall(benchmark::State &state)
{
  ECDSASigner const signer;

  // create the transaction store
  TransactionStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(state.max_iterations, signer, false);

  std::size_t tx_index{0};
  for (auto _ : state)
  {
    auto const &tx = transactions.at(tx_index++);
    tx_store.Add(*tx);
  }
}

void TxSubmitSingleSmallAlt(benchmark::State &state)
{
  ECDSASigner const signer;

  // create the transaction store
  TransactionStorageEngine tx_store{LOG2_NUM_LANES, LANE_ID};
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(state.max_iterations, signer, false);

  std::size_t tx_index{0};
  for (auto _ : state)
  {
    auto const &tx = transactions.at(tx_index++);
    tx_store.Add(*tx, false);
  }
}

void TransientStoreExpectedOperation(benchmark::State &state)
{
  ECDSASigner const signer;

  // create the transient store
  TransactionStorageEngine tx_store{LOG2_NUM_LANES, LANE_ID};
  tx_store.New("transaction.db", "transaction_index.db", true);

  Transaction dummy;

  for (auto _ : state)
  {
    state.PauseTiming();
    // Number of Tx to send is state arg
    TransactionList transactions = GenerateTransactions(std::size_t(state.range(0)), signer, true);
    state.ResumeTiming();

    for (auto const &tx : transactions)
    {
      auto const &digest{tx->digest()};

      // Basic pattern for a transient store is to intake X transactions into the mempool,
      // then read some subset N of them (for block verification/packing), then commit
      // those to the underlying object store.
      tx_store.Add(*tx, false);

      // also trigger the read from the store and the subsequent right schedule
      tx_store.Get(digest, dummy);
      tx_store.Confirm(digest);

      benchmark::DoNotOptimize(dummy);
    }
  }
}

}  // namespace

BENCHMARK(TransientStoreExpectedOperation)->Range(10, 1000000);
BENCHMARK(TxSubmitSingleSmallAlt);
BENCHMARK(TxSubmitFixedLarge);
BENCHMARK(TxSubmitFixedSmall);
BENCHMARK(TxSubmitSingleLarge);
BENCHMARK(TxSubmitSingleSmall);
