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

#include "benchmark/benchmark.h"

#include "tx_generation.hpp"

#include <vector>

using fetch::storage::ResourceID;
using fetch::chain::Transaction;
using fetch::chain::TransactionBuilder;
using fetch::crypto::ECDSASigner;

using ObjectStore      = fetch::storage::ObjectStore<Transaction>;
using TransactionStore = fetch::storage::TransientObjectStore<Transaction>;
using TransactionList  = std::vector<TransactionBuilder::TransactionPtr>;

void TxSubmitWrites(benchmark::State &state)
{
  ECDSASigner const signer;

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
    TransactionList transactions =
        GenerateTransactions(std::size_t(state.range(1)), signer, small_tx);
    state.ResumeTiming();

    for (auto const &tx : transactions)
    {
      store.Set(ResourceID{tx->digest()}, *tx);
    }

    // For a fair test must force flush to disk after writes - note this makes the results for small
    // tx writes very poor
    store.Flush(false);
  }
}

BENCHMARK(TxSubmitWrites)->Ranges({{0, 1}, {1, 1000000}});
