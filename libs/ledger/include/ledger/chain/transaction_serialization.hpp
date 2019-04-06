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

#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"

namespace fetch {
namespace ledger {

template <typename T>
inline void Serialize(T &serializer, UnverifiedTransaction const &b)
{
  serializer << uint16_t(b.VERSION);
  serializer << ' ';
  serializer << b.summary();
  serializer << b.data();
  serializer << b.signatures();
  serializer << b.contract_name();
}

template <typename T>
inline void Deserialize(T &serializer, UnverifiedTransaction &b)
{
  uint16_t version;
  serializer >> version;  // TODO(issue 34): set version

  char c;
  serializer >> c;

  TransactionSummary    summary;
  std::string           contract_name;
  byte_array::ByteArray data;
  Signatories           signatures;

  serializer >> summary;
  b.set_summary(summary);

  serializer >> data;
  b.set_data(data);

  serializer >> signatures;
  b.set_signatures(signatures);

  serializer >> contract_name;
  b.set_contract_name(contract_name);
}

template <typename T>
inline void Serialize(T &serializer, VerifiedTransaction const &b)
{
  serializer << uint16_t(b.VERSION);
  serializer << 'V';
  serializer << b.summary();
  serializer << b.data();
  serializer << b.signatures();
  serializer << b.contract_name();
}

template <typename T>
inline void Deserialize(T &serializer, VerifiedTransaction &b)
{
  uint16_t version;
  serializer >> version;  // TODO(issue 34): set version
  assert(version == b.VERSION);

  char c;
  serializer >> c;
  assert(c == 'V');

  TransactionSummary    summary;
  std::string           contract_name;
  byte_array::ByteArray data;
  Signatories           signatures;

  serializer >> summary;
  b.set_summary(summary);

  serializer >> data;
  b.set_data(data);

  serializer >> signatures;
  b.set_signatures(signatures);

  serializer >> contract_name;
  b.set_contract_name(contract_name);
}

}  // namespace ledger
}  // namespace fetch
