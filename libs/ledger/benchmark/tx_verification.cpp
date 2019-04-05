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
#include "ledger/chain/transaction.hpp"

#include <benchmark/benchmark.h>

using fetch::ledger::MutableTransaction;
using fetch::crypto::ECDSASigner;

namespace {

void VerifyTx(benchmark::State &state)
{
  ECDSASigner        signer;
  MutableTransaction mtx;
  mtx.set_contract_name("foo.bar.is.a.baz");
  mtx.Sign(signer.underlying_private_key());

  for (auto _ : state)
  {
    mtx.Verify();
  }
}

}  // namespace

BENCHMARK(VerifyTx);