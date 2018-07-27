#pragma once

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
