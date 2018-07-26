#pragma once

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
  uint16_t version;
  serializer >> version; // TODO: (`HUT`) : set version

  char c;
  serializer >> c;
  
  TransactionSummary summary;
  byte_array::ByteArray data, signature;
  std::string contract_name;
  
  serializer >> summary;
  b.set_summary(summary);

  serializer >> data;
  b.set_data(data);
  
  serializer >> signature;
  b.set_signature(signature);

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
  serializer << b.signature();
  serializer << b.contract_name();
}

template <typename T>
inline void Deserialize(T &serializer, VerifiedTransaction &b)
{
  uint16_t version;
  serializer >> version; // TODO: (`HUT`) : set version
  assert(version == b.VERSION);
  
  char c;
  serializer >> c;
  assert( c == 'V' );  
  
  TransactionSummary summary;
  byte_array::ByteArray data, signature;
  std::string contract_name;
  
  serializer >> summary;
  b.set_summary(summary);

  serializer >> data;
  b.set_data(data);
  
  serializer >> signature;
  b.set_signature(signature);

  serializer >> contract_name;
  b.set_contract_name(contract_name);
}

}
}
