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
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"

#include "tx_generation.hpp"

#include <benchmark/benchmark.h>
#include <condition_variable>
#include <thread>

using fetch::ledger::TransactionVerifier;
using fetch::crypto::ECDSASigner;

namespace {

class DummySink : public fetch::ledger::TransactionSink
{

  std::size_t const       threshold_;
  std::size_t             count_{0};
  std::mutex              lock_;
  std::condition_variable condition_;

public:
  explicit DummySink(std::size_t threshold)
    : threshold_(threshold)
  {}

  void OnTransaction(TransactionPtr const &) override
  {
    std::lock_guard<std::mutex> lock(lock_);
    ++count_;

    if (count_ >= threshold_)
    {
      condition_.notify_all();
    }
  }

  void Wait()
  {
    std::unique_lock<std::mutex> lock(lock_);
    if (count_ >= threshold_)
    {
      return;
    }

    condition_.wait(lock);
  }
};

void TransactionVerifierBench(benchmark::State &state)
{
  //  std::cout << "Tx Verification - threads: " << state.range(0) << " num txs: " << state.range(1)
  //  << std::endl;

  // generate the transactions
  ECDSASigner signer;
  auto const  txs = GenerateTransactions(static_cast<std::size_t>(state.range(1)), signer);

  // wait for the
  for (auto _ : state)
  {
    state.PauseTiming();

    DummySink sink{txs.size()};

    // needs to be created on the heap because of memory use
    auto verifier = std::make_unique<TransactionVerifier>(sink, state.range(0), "Verifier");

    // front load the verifier
    for (auto const &tx : txs)
    {
      verifier->AddTransaction(tx);
    }

    verifier->Start();
    state.ResumeTiming();

    // wait for the
    sink.Wait();

    state.PauseTiming();
    verifier->Stop();
  }
}

void CreateRanges(benchmark::internal::Benchmark *b)
{
  int const max_threads = static_cast<int>(std::thread::hardware_concurrency());

  for (int i = 1; i <= max_threads; ++i)
  {
    b->Args({i, 1});
    b->Args({i, 10});
    b->Args({i, 100});
    b->Args({i, 1000});
    b->Args({i, 10000});
    b->Args({i, 100000});
  }
}

}  // namespace

BENCHMARK(TransactionVerifierBench)->Apply(CreateRanges);
