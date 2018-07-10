#ifndef CHAIN_MUTABLE_TRANSACTION_HPP
#define CHAIN_MUTABLE_TRANSACTION_HPP

#include "ledger/chain/basic_transaction.hpp"
#include "ledger/chain/transaction.hpp"

namespace fetch {

namespace chain {

class MutableTransaction : public BasicTransaction {
public:
  typedef BasicTransaction super_type;

  using super_type::PushGroup;
  using super_type::set_contract_name;
  using super_type::set_data;
  using super_type::set_summary;
  using super_type::set_signature;

  static Transaction MakeTransaction(MutableTransaction &&);
};

// Conversion to Transaction
inline Transaction MutableTransaction::MakeTransaction(MutableTransaction &&trans)
{
  Transaction ret(std::move(trans));
  return ret;
}

}
}
#endif
