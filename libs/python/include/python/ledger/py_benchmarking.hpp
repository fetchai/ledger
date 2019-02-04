#pragma once
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

#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/serializers/stl_types.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "vectorise/threading/pool.hpp"

#include <cstddef>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace fetch {
namespace ledger {

struct AdaptedTx
{
  ledger::MutableTransaction                           tx;
  ledger::TxSigningAdapter<ledger::MutableTransaction> adapter{tx};

  template <typename T>
  friend void Serialize(T &s, AdaptedTx const &a)
  {
    s << a.adapter;
  }
};

pybind11::bytes CreateWealthTransactionsBasic(std::size_t num_transactions)
{
  // generate a series of keys for all the nodes
  std::vector<crypto::ECDSASigner> signers(num_transactions);

  // create all the transactions
  std::vector<AdaptedTx> transactions(num_transactions);
  for (std::size_t i = 0; i < num_transactions; ++i)
  {
    auto &identity = signers.at(i);
    auto &tx       = transactions.at(i);

    auto const public_key = identity.public_key();

    std::ostringstream oss;
    oss << R"( { "address": ")" << byte_array::ToBase64(public_key) << R"(", "amount": 10 })";

    tx.tx.set_contract_name("fetch.token.wealth");
    tx.tx.set_fee(1);
    tx.tx.set_data(oss.str());
    tx.tx.set_resources({public_key});
    tx.tx.Sign(identity.underlying_private_key());
  }

  // convert to a serial stream
  serializers::ByteArrayBuffer buffer;
  buffer.Append(transactions);

  return {buffer.data().char_pointer(), buffer.data().size()};
}

pybind11::bytes CreateWealthTransactionsThreaded(std::size_t num_transactions)
{
  threading::Pool pool;

  // generate a series of keys for all the nodes
  std::mutex                       signers_mtx;
  std::vector<crypto::ECDSASigner> signers(0);
  for (std::size_t i = 0; i < num_transactions; ++i)
  {
    pool.Dispatch([&signers_mtx, &signers]() {
      crypto::ECDSASigner identity;

      {
        std::lock_guard<std::mutex> lock(signers_mtx);
        signers.emplace_back(std::move(identity));
      }
    });
  }

  pool.Wait();

  // create all the transactions
  std::vector<AdaptedTx> transactions(num_transactions);
  for (std::size_t i = 0; i < num_transactions; ++i)
  {
    pool.Dispatch([i, &transactions, &signers]() {
      auto &identity = signers.at(i);
      auto &tx       = transactions.at(i);

      auto const public_key = identity.public_key();

      std::ostringstream oss;
      oss << R"( { "address": ")" << byte_array::ToBase64(public_key) << R"(", "amount": 10 })";

      tx.tx.set_contract_name("fetch.token.wealth");
      tx.tx.set_fee(1);
      tx.tx.set_data(oss.str());
      tx.tx.set_resources({public_key});
      tx.tx.Sign(identity.underlying_private_key());
    });
  }

  pool.Wait();

  // convert to a serial stream
  serializers::ByteArrayBuffer buffer;
  buffer.Append(transactions);

  return {buffer.data().char_pointer(), buffer.data().size()};
}

pybind11::bytes CreateWealthTransactions(std::size_t num_transactions)
{
  if (num_transactions > 1000)
  {
    return CreateWealthTransactionsThreaded(num_transactions);
  }
  else
  {
    return CreateWealthTransactionsBasic(num_transactions);
  }
}

void BuildBenchmarking(pybind11::module &module)
{
  module.def("create_wealth_txs", CreateWealthTransactions);
}

}  // namespace ledger
}  // namespace fetch
