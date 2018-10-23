//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <benchmark/benchmark.h>
#include <gtest/gtest.h>

#include "core/random/lfg.hpp"
#include "storage/object_store.hpp"

#include "ledger/chaincode/wallet_http_interface.hpp"

using namespace fetch;
using namespace fetch::storage;
using namespace fetch::chain;

class ObjectStoreBench : public ::benchmark::Fixture
{
protected:
  void SetUp(const ::benchmark::State &st) override
  {
    store_.New("obj_store_bench.db", "obj_store_bench_index.db");

    for (std::size_t i = 0; i < 10000; ++i)
    {
      CreateNextTransaction();
    }
  }

  void TearDown(const ::benchmark::State &) override
  {}

  std::vector<Transaction>                  precreated_tx_;
  std::vector<ResourceID>                   precreated_rid_;
  ObjectStore<Transaction>                  store_;
  fetch::random::LaggedFibonacciGenerator<> lfg_;

private:
  // Create dummy transactions for testing
  void CreateNextTransaction()
  {
    static int  amount = 0;
    std::string address =
        "nve3rfRigBZ+YX9lFKVNDA05fQ6V8MfEmx63+bKT0IQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    script::Variant wealth_data;
    wealth_data.MakeObject();
    wealth_data["address"] = address;
    wealth_data["amount"]  = amount++;

    std::ostringstream oss;
    oss << wealth_data;

    chain::MutableTransaction mtx;

    mtx.set_contract_name("fetch.token.wealth");
    mtx.set_data(oss.str());
    mtx.set_fee(lfg_() & 0x1FF);
    mtx.PushResource(address);

    /*
    // sign the transaction
    crypto::ECDSASigner          signer;
    signer.GenerateKeys();
    mtx.Sign(signer.private_key()); */

    precreated_tx_.push_back(chain::VerifiedTransaction::Create(std::move(mtx)));
    // precreated_rid_.push_back(ResourceAddress{signer.public_key()});
    precreated_rid_.push_back(ResourceAddress{std::to_string(amount)});
  }
};

BENCHMARK_F(ObjectStoreBench, WritingTxToStore_1)(benchmark::State &st)
{
  std::size_t counter = 0;
  for (auto _ : st)
  {
    std::size_t mod_counter = counter % precreated_tx_.size();
    store_.Set(precreated_rid_[mod_counter], precreated_tx_[mod_counter]);
    counter++;
  }
}

BENCHMARK_F(ObjectStoreBench, WritingTxToStore_10)(benchmark::State &st)
{
  std::size_t counter = 0;
  for (auto _ : st)
  {
    for (std::size_t i = 0; i < 10; ++i)
    {
      std::size_t mod_counter = counter % precreated_tx_.size();
      store_.Set(precreated_rid_[mod_counter], precreated_tx_[mod_counter]);
      counter++;
    }
  }
}

BENCHMARK_F(ObjectStoreBench, WritingTxToStore_1k)(benchmark::State &st)
{
  std::size_t counter = 0;
  for (auto _ : st)
  {
    for (std::size_t i = 0; i < 1000; ++i)
    {
      std::size_t mod_counter = counter % precreated_tx_.size();
      store_.Set(precreated_rid_[mod_counter], precreated_tx_[mod_counter]);
      counter++;
    }
  }
}

BENCHMARK_F(ObjectStoreBench, WritingTxToStore_10k)(benchmark::State &st)
{
  std::size_t counter = 0;
  for (auto _ : st)
  {
    for (std::size_t i = 0; i < 10000; ++i)
    {
      std::size_t mod_counter = counter % precreated_tx_.size();
      store_.Set(precreated_rid_[mod_counter], precreated_tx_[mod_counter]);
      counter++;
    }
  }
}

BENCHMARK_F(ObjectStoreBench, RdWrTxToStore_10k)(benchmark::State &st)
{
  std::size_t counter = 0;
  for (auto _ : st)
  {
    for (std::size_t i = 0; i < 10000; ++i)
    {
      std::size_t mod_counter = counter % precreated_tx_.size();
      store_.Set(precreated_rid_[mod_counter], precreated_tx_[mod_counter]);
      counter++;
    }

    std::random_shuffle(precreated_rid_.begin(), precreated_rid_.end());
    counter = 0;

    for (std::size_t i = 0; i < 10000; ++i)
    {
      Transaction dummy;
      std::size_t mod_counter = counter % precreated_tx_.size();
      benchmark::DoNotOptimize(store_.Get(precreated_rid_[mod_counter], dummy));
      // 12888964394
      // 12221606575
      counter++;
    }
  }
}

// Macro required for all benchmarks
BENCHMARK_MAIN();
