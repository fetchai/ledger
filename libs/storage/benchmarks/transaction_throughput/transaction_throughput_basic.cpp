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
#include "ledger/storage_unit/lane_service.hpp"
#include "storage/transient_object_store.hpp"

#include <benchmark/benchmark.h>
#include <vector>

using fetch::byte_array::ByteArray;
using fetch::storage::ResourceID;
using fetch::ledger::VerifiedTransaction;
using fetch::ledger::MutableTransaction;
using fetch::random::LinearCongruentialGenerator;

using ObjectStore      = fetch::storage::ObjectStore<VerifiedTransaction>;
using TransactionStore = fetch::storage::TransientObjectStore<VerifiedTransaction>;
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

void TxSubmitWrites(benchmark::State &state)
{
  // create the store
  ObjectStore store;
  store.New("transaction.db", "transaction_index.db", true);

  bool small_tx = state.range(0) == 0;

  std::string number_of_tx;
  {
    std::ostringstream oss;
    oss << std::setprecision(3) << std::scientific << float(state.range(1)) << " Tx,";
    number_of_tx = oss.str();
  }

  if (small_tx)
  {
    number_of_tx += " small_tx";
    state.SetLabel(number_of_tx);
  }
  else
  {
    number_of_tx += " large_tx";
    state.SetLabel(number_of_tx);
  }

  for (auto _ : state)
  {
    // create X new unique transactions to write per test
    state.PauseTiming();
    TransactionList transactions = GenerateTransactions(std::size_t(state.range(1)), small_tx);
    state.ResumeTiming();

    for (auto const &tx : transactions)
    {
      store.Set(ResourceID{tx.digest()}, tx);
    }

    // For a fair test must force flush to disk after writes - note this makes the results for small
    // tx writes very poor
    store.Flush(false);
  }
}

BENCHMARK(TxSubmitWrites)->Ranges({{0, 1}, {1, 1000000}});

BENCHMARK_MAIN();
