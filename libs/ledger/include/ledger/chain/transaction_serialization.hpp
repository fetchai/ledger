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

#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"

namespace fetch {
namespace chain {

template <typename T>
inline void Serialize(T &serializer, UnverifiedTransaction const &b)
{
  serializer << uint16_t(b.VERSION);
  serializer << ' ';
  serializer << b.summary();
  serializer << b.data();
  serializer << b.signature();
  serializer << b.contract_name();
}

template <typename T>
inline void Deserialize(T &serializer, UnverifiedTransaction &b)
{
  FETCH_LOG_INFO(LOGGING_NAME,">>UnverifiedTransaction 1");
  uint16_t version;
  serializer >> version;  // TODO(issue 34): set version

  FETCH_LOG_INFO(LOGGING_NAME,">>UnverifiedTransaction 2");
  char c;
  serializer >> c;

  FETCH_LOG_INFO(LOGGING_NAME,">>UnverifiedTransaction 3");
  TransactionSummary    summary;
  byte_array::ByteArray data, signature;
  std::string           contract_name;

  FETCH_LOG_INFO(LOGGING_NAME,">>UnverifiedTransaction 4");
  serializer >> summary;
  b.set_summary(summary);

  FETCH_LOG_INFO(LOGGING_NAME,">>UnverifiedTransaction 5");
  serializer >> data;
  b.set_data(data);

  FETCH_LOG_INFO(LOGGING_NAME,">>UnverifiedTransaction 6");
  serializer >> signature;
  b.set_signature(signature);

  FETCH_LOG_INFO(LOGGING_NAME,">>UnverifiedTransaction 7");
  serializer >> contract_name;
  b.set_contract_name(contract_name);

  FETCH_LOG_INFO(LOGGING_NAME,">>UnverifiedTransaction X");
}

template <typename T>
inline void Serialize(T &serializer, VerifiedTransaction const &b)
{
  serializer << uint16_t(b.VERSION);
  serializer << 'V';
  serializer << b.summary();
  serializer << b.data();
  serializer << b.signature();
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
  byte_array::ByteArray data, signature;
  std::string           contract_name;

  serializer >> summary;
  b.set_summary(summary);

  serializer >> data;
  b.set_data(data);

  serializer >> signature;
  b.set_signature(signature);

  serializer >> contract_name;
  b.set_contract_name(contract_name);
}

}  // namespace chain
}  // namespace fetch
