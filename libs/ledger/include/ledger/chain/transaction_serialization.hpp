#ifndef CHAIN_TRANSACTION_SERIALIZATION_HPP
#define CHAIN_TRANSACTION_SERIALIZATION_HPP

#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"

namespace fetch {
namespace chain {


template <typename T>
inline void Serialize(T &serializer, Transaction const &b)
{
  serializer << uint16_t(b.VERSION);
  serializer << b.summary_;
  serializer << b.data_;
  serializer << b.signature_;
  serializer << b.contract_name_.full_name();
}

template <typename T>
inline void Deserialize(T &serializer, Transaction &b)
{
  uint16_t version;
  serializer >> version; // TODO: (`HUT`) : set version

  serializer >> b.summary_;
  serializer >> b.data_;
  serializer >> b.signature_;

  std::string name;
  serializer >> name;
  b.contract_name_.Parse(std::move(name));
}

}
}
#endif
