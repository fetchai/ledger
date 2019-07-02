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

#include "core/byte_array/const_byte_array.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serializer.hpp"

namespace fetch {
namespace serializers {

template< typename D >
struct ForwardSerializer< ledger::Transaction, D >
{
public:
  using Type = ledger::Transaction;
  using DriverType = D;

  template< typename Serializer >
  static inline void Serialize(Serializer &s, Type const &tx)
  {
    ledger::TransactionSerializer serializer{};
    serializer << tx;

    s << serializer.data();
  }

  template< typename Serializer >
  static inline void Deserialize(Serializer &s, Type &tx)
  {
    // extract the data from the stream
    byte_array::ConstByteArray data;
    s >> data;

    // create and extract the serializer
    ledger::TransactionSerializer serializer{data};
    serializer >> tx;
  }
};


}  // namespace ledger
}  // namespace fetch
