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
  serializer << b.summary();
  serializer << b.signature();
  serializer << b.contract_name().full_name();
}

template <typename T>
inline void Deserialize(T &serializer, Transaction &b)
{
  uint16_t version;
  serializer >> version; // TODO: (`HUT`) : set version

  TransactionSummary summary;
  serializer >> summary;
  b.set_summary(summary);

  serializer >> b.signature();

  std::string name;
  serializer >> name;
  b.contract_name().Parse(std::move(name));
}

}
}
#endif
