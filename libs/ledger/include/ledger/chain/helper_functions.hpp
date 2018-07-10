#ifndef TRANSACTION_HELPER_FUNCTIONS_HPP
#define TRANSACTION_HELPER_FUNCTIONS_HPP

#include<random>
#include"core/byte_array/referenced_byte_array.hpp"
#include"ledger/chain/basic_transaction.hpp"
#include"ledger/chain/transaction.hpp"
#include"ledger/chain/mutable_transaction.hpp"
#include"ledger/chain/transaction_serialization.hpp"

namespace fetch {
namespace chain {

uint64_t GetRandom()
{
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint64_t> dis(
      0, std::numeric_limits<uint64_t>::max());
  return dis(gen);
}

byte_array::BasicByteArray GetRandomByteArray()
{
  return byte_array::BasicByteArray(std::to_string(GetRandom()));
}

MutableTransaction RandomTransaction(std::size_t bytesToAdd = 0)
{
  MutableTransaction trans;
  TransactionSummary summary;

  for (std::size_t i = 0; i < 3; ++i)
  {
    summary.groups.push_back(GetRandomByteArray());
  }

  summary.fee = GetRandom();
  trans.set_summary(summary);

  trans.set_data(GetRandomByteArray());
  trans.signature() = GetRandomByteArray();
  trans.contract_name().Parse(std::to_string(GetRandom()));

  return trans;
}

}
}

#endif
