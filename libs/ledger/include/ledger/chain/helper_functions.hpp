#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include <random>

namespace fetch {
namespace chain {

uint64_t GetRandom()
{
  static std::random_device                      rd;
  static std::mt19937                            gen(rd());
  static std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());
  return dis(gen);
}

byte_array::ConstByteArray GetRandomByteArray() { return {std::to_string(GetRandom())}; }

MutableTransaction RandomTransaction(std::size_t bytesToAdd = 0)
{
  MutableTransaction trans;
  TransactionSummary summary;

  for (std::size_t i = 0; i < 3; ++i)
  {
    summary.resources.insert(GetRandomByteArray());
  }

  summary.fee = GetRandom();
  trans.set_summary(summary);

  trans.set_data(GetRandomByteArray());
  trans.set_signature(GetRandomByteArray());
  trans.set_contract_name(std::to_string(GetRandom()));

  return trans;
}

}  // namespace chain
}  // namespace fetch
