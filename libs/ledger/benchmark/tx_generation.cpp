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

#include "crypto/ecdsa.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "vectorise/threading/pool.hpp"

#include <benchmark/benchmark.h>
#include <memory>

using fetch::ledger::MutableTransaction;
using fetch::ledger::TxSigningAdapter;
using fetch::crypto::ECDSASigner;
using fetch::byte_array::ToBase64;
using fetch::serializers::ByteArrayBuffer;
using fetch::threading::Pool;

namespace {

struct AdaptedTx
{
  MutableTransaction                   tx;
  TxSigningAdapter<MutableTransaction> adapter{tx};

  template <typename T>
  friend void Serialize(T &s, AdaptedTx const &a)
  {
    s << a.adapter;
  }
};

static const std::size_t NUM_TRANSACTIONS = 50000;

void TxGeneration(benchmark::State &state)
{

  for (auto _ : state)
  {
    // generate a series of keys for all the nodes
    std::vector<ECDSASigner> signers(NUM_TRANSACTIONS);

    // create all the transactions
    std::vector<AdaptedTx> transactions(NUM_TRANSACTIONS);
    for (std::size_t i = 0; i < NUM_TRANSACTIONS; ++i)
    {
      auto &identity = signers.at(i);
      auto &tx       = transactions.at(i);

      auto const public_key = identity.public_key();

      std::ostringstream oss;
      oss << R"( { "address": ")" << ToBase64(public_key) << R"(", "amount": 10 )";

      tx.tx.set_contract_name("fetch.token.wealth");
      tx.tx.set_fee(1);
      tx.tx.set_data(oss.str());
      tx.tx.set_resources({public_key});
      tx.tx.Sign(identity.private_key());
    }

    // convert to a serial stream
    ByteArrayBuffer buffer;
    buffer.Append(transactions);
  }
}

void TxGenerationThreaded(benchmark::State &state)
{
  using SignerPtr = std::unique_ptr<ECDSASigner>;

  for (auto _ : state)
  {
    Pool pool;

    // generate a series of keys for all the nodes
    std::mutex             signers_mtx;
    std::vector<SignerPtr> signers(0);
    for (std::size_t i = 0; i < NUM_TRANSACTIONS; ++i)
    {
      pool.Dispatch([&signers_mtx, &signers]() {
        auto identity = std::make_unique<ECDSASigner>();

        {
          FETCH_LOCK(signers_mtx);
          signers.emplace_back(std::move(identity));
        }
      });
    }

    pool.Wait();

    // create all the transactions
    std::vector<AdaptedTx> transactions(NUM_TRANSACTIONS);
    for (std::size_t i = 0; i < NUM_TRANSACTIONS; ++i)
    {
      pool.Dispatch([i, &transactions, &signers]() {
        auto &identity = *signers.at(i);
        auto &tx       = transactions.at(i);

        auto const public_key = identity.public_key();

        std::ostringstream oss;
        oss << R"( { "address": ")" << ToBase64(public_key) << R"(", "amount": 10 )";

        tx.tx.set_contract_name("fetch.token.wealth");
        tx.tx.set_fee(1);
        tx.tx.set_data(oss.str());
        tx.tx.set_resources({public_key});
        tx.tx.Sign(identity.private_key());
      });
    }

    pool.Wait();

    // convert to a serial stream
    ByteArrayBuffer buffer;
    buffer.Append(transactions);
  }
}

}  // namespace

BENCHMARK(TxGeneration);
BENCHMARK(TxGenerationThreaded);
