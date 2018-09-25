#pragma once
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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include <random>

namespace fetch {
namespace chain {

inline uint64_t GetRandom()
{
  static std::random_device                      rd;
  static std::mt19937                            gen(rd());
  static std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());
  return dis(gen);
}

inline byte_array::ConstByteArray GetRandomByteArray()
{
  return {std::to_string(GetRandom())};
}

inline MutableTransaction RandomTransaction(std::size_t bytesToAdd = 0)
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
  trans.set_contract_name(std::to_string(GetRandom()));
 
  uint8_t const   size = static_cast<uint8_t>(GetRandom() % 3 + 1);
  for (uint8_t i = 0; i < size; ++i)
  {
    crypto::ECDSASigner::PrivateKey key;
    trans.Sign(key.KeyAsBin());
  }
  trans.UpdateDigest();
  return trans;
}

inline std::ostream& operator << (std::ostream &os, MutableTransaction const& transaction)
{
  os << "contract name:   " << transaction.contract_name() << std::endl;
  os << "hash:            " << byte_array::ToHex(transaction.summary().transaction_hash) << std::endl;
  os << "data:            " << byte_array::ToHex(transaction.data()) << std::endl;

  os << "=== Resources ===========================================" << std::endl;
  for (auto const &res : transaction.resources())
  {
    os << "resource:        " << byte_array::ToHex(res) << std::endl;
  }

  os << "=== Signatures ==========================================" << std::endl;
  for (auto const &sig : transaction.signatures())
  {
    os << "identity:        " << byte_array::ToHex(sig.first.identifier()) << std::endl;
    os << "identity params: " << sig.first.parameters() << std::endl;
    os << "signature:       " << byte_array::ToHex(sig.second.signature_data) << std::endl;
    os << "signature type:  " << sig.second.type << std::endl;
  }
  //os << "=========================================================" << std::endl;
  return os;
}

}  // namespace chain
}  // namespace fetch
