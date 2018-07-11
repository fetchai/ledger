#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP

#include "ledger/chain/mutable_transaction.hpp"

namespace fetch {
namespace chain {

class Transaction : private MutableTransaction
{
  
public:
  typedef MutableTransaction super_type;  

  Transaction(Transaction const &other) = default;
  Transaction(Transaction &&other) = default;

  using super_type::groups;
  using super_type::summary;
  using super_type::data;
  using super_type::signature;  
  using super_type::contract_name;    
  
  operator MutableTransaction() 
  {
    MutableTransaction ret;
    ret.Copy( *this );    
  }
  
  bool Finalise( MutableTransaction &base ) 
  {
    base.copy_on_write_ = true;    
    this->Copy(base);
    UpdateDigest();
    return Verify();
  }
   
private:

  template <typename T>
  friend inline void Serialize(T &, Transaction const &);
  template <typename T>
  friend inline void Deserialize(T &, Transaction &);
};

} // namespace chain
} // namespace fetch

#endif
