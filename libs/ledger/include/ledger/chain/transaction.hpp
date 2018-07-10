#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP

#include "ledger/chain/basic_transaction.hpp"

namespace fetch
{

namespace chain
{

class Transaction : public BasicTransaction
{
public:
  typedef BasicTransaction super_type;

  Transaction() = default;

  explicit Transaction(super_type &&super) : super_type(super) 
  {
    UpdateDigest();
  }

  bool operator==(const BasicTransaction &rhs) const { return super_type::operator==(rhs); }
  bool operator<(const BasicTransaction &rhs) const { return super_type::operator<(rhs);  }

  std::vector<group_type> const &groups() const
  {
    return super_type::groups();
  }

  byte_array::ConstByteArray const &signature() const
  {
    return super_type::signature();
  }

  ledger::Identifier const &contract_name() const
  {
    return super_type::contract_name();
  }

  digest_type const &digest() const
  {
    return super_type::digest();
  }

  byte_array::ConstByteArray data() const
  {
    return super_type::data();
  };

  TransactionSummary const &summary() const
  {
    return super_type::summary();
  }

private:

  byte_array::ConstByteArray &signature() { return super_type::signature(); }
  ledger::Identifier &contract_name() { return super_type::contract_name(); }

  template <typename T>
  friend inline void Serialize(T &, Transaction const &);
  template <typename T>
  friend inline void Deserialize(T &, Transaction &);
};

}
}

#endif
