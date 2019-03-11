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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/random/lcg.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "storage/transient_object_store.hpp"

#include <benchmark/benchmark.h>
#include <vector>

namespace {

using fetch::byte_array::ByteArray;
using fetch::storage::ResourceID;
using fetch::ledger::VerifiedTransaction;
using fetch::ledger::MutableTransaction;
using fetch::random::LinearCongruentialGenerator;

using TransientStore   = fetch::storage::TransientObjectStore<VerifiedTransaction>;
using TransactionStore = fetch::storage::ObjectStore<VerifiedTransaction>;
using TransactionList  = std::vector<VerifiedTransaction>;

TransactionList GenerateTransactions(std::size_t count, bool large_packets)
{
  static constexpr std::size_t TX_SIZE      = 2048;
  static constexpr std::size_t TX_WORD_SIZE = TX_SIZE / sizeof(uint64_t);

  static_assert((TX_SIZE % sizeof(uint64_t)) == 0, "The transaction must be a multiple of 64bits");

  static LinearCongruentialGenerator rng;

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

    list.emplace_back(VerifiedTransaction::Create(std::move(mtx)));
  }

  return list;
}

void TxSubmitFixedLarge(benchmark::State &state)
{
  // create the transaction store
  TransactionStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(50000, true);

  for (auto _ : state)
  {
    for (auto const &tx : transactions)
    {
      tx_store.Set(ResourceID{tx.digest()}, tx);
    }
  }
}

void TxSubmitFixedSmall(benchmark::State &state)
{
  // create the transaction store
  TransactionStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(50000, false);

  for (auto _ : state)
  {
    for (auto const &tx : transactions)
    {
      tx_store.Set(ResourceID{tx.digest()}, tx);
    }
  }
}

void TxSubmitSingleLarge(benchmark::State &state)
{
  // create the transaction store
  TransactionStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(state.max_iterations, true);

  std::size_t tx_index{0};
  for (auto _ : state)
  {
    auto const &tx = transactions.at(tx_index++);
    tx_store.Set(ResourceID{tx.digest()}, tx);
  }
}

void TxSubmitSingleSmall(benchmark::State &state)
{
  // create the transaction store
  TransactionStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(state.max_iterations, false);

  std::size_t tx_index{0};
  for (auto _ : state)
  {
    auto const &tx = transactions.at(tx_index++);
    tx_store.Set(ResourceID{tx.digest()}, tx);
  }
}

void TxSubmitSingleSmallAlt(benchmark::State &state)
{
  // create the transaction store
  TransientStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  // create a whole series of transaction
  TransactionList transactions = GenerateTransactions(state.max_iterations, false);

  std::size_t tx_index{0};
  for (auto _ : state)
  {
    auto const &tx = transactions.at(tx_index++);
    tx_store.Set(ResourceID{tx.digest()}, tx, false);
  }
}

void TransientStoreExpectedOperation(benchmark::State &state)
{
  // create the transient store
  TransientStore tx_store;
  tx_store.New("transaction.db", "transaction_index.db", true);

  VerifiedTransaction dummy;

  for (auto _ : state)
  {
    state.PauseTiming();
    // Number of Tx to send is state arg
    TransactionList transactions = GenerateTransactions(size_t(state.range(0)), true);
    state.ResumeTiming();

    for (auto const &tx : transactions)
    {
      ResourceID const rid{tx.digest()};

      // Basic pattern for a transient store is to intake X transactions into the mempool,
      // then read some subset N of them (for block verification/packing), then commit
      // those to the underlying object store.
      tx_store.Set(ResourceID{tx.digest()}, tx, false);

      // also trigger the read from the store and the subsequent right schedule
      tx_store.Get(rid, dummy);
      tx_store.Confirm(rid);

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
