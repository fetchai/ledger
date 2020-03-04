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

#include "tx_generation.hpp"

#include "chain/transaction_serializer.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/ecdsa.hpp"

#include "benchmark/benchmark.h"

#include <vector>

using fetch::chain::TransactionSerializer;
using fetch::crypto::ECDSASigner;
using Storage      = std::vector<fetch::byte_array::ConstByteArray>;
using Transactions = std::vector<fetch::chain::Transaction>;

void TxSerialisation(benchmark::State &state)
{
  state.PauseTiming();

  ECDSASigner const signer;
  // create the testing set
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

    TransactionList input = GenerateTransactions(std::size_t(state.range(1)), signer, small_tx);

    const auto            num_txs = input.size();
    Transactions          output(num_txs);
    Storage               cells(num_txs);
    TransactionSerializer sr;
    std::size_t           in_errors{}, out_errors{};

    state.ResumeTiming();
    for (std::size_t i{}; i < num_txs; ++i)
    {
      if (sr.Serialize(input[i]))
      {
        cells[i] = sr.data();
      }
      else
      {
        ++in_errors;
      }
    }
    for (std::size_t i{}; i < num_txs; ++i)
    {
      if (!cells[i].empty())
      {
        if (!sr.Deserialize(output[i]))
        {
          ++out_errors;
        }
      }
    }
    state.PauseTiming();
    // TODO (nobody): we've got a good randomized test to check that transactions match
  }
}

BENCHMARK(TxSerialisation)->Ranges({{0, 1}, {1, 1000000}});
