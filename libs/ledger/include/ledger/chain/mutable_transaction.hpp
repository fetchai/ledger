#ifndef CHAIN_MUTABLE_TRANSACTION_HPP
#define CHAIN_MUTABLE_TRANSACTION_HPP

#include "ledger/chain/basic_transaction.hpp"

namespace fetch {

namespace chain {

class MutableTransaction : public BasicTransaction {
public:
  typedef BasicTransaction super_type;

  using super_type::operator==;
  using super_type::operator<;
  using super_type::PushGroup;
  using super_type::set_contract_name;
  using super_type::groups;
  using super_type::contract_name;
  using super_type::data;
  using super_type::set_data;
  using super_type::summary;
  using super_type::set_summary;

  using super_type::UpdateDigest;

  static Transaction MakeTransaction(MutableTransaction &&);
};

// Conversion to Transaction
Transaction MutableTransaction::MakeTransaction(MutableTransaction &&trans)
{
  Transaction ret(std::move(trans));
  return ret;
}

}
}
#endif
