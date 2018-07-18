#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP

#include "ledger/chain/mutable_transaction.hpp"

namespace fetch {
namespace chain {

class UnverifiedTransaction : private MutableTransaction 
{
public:
  typedef MutableTransaction super_type;
  UnverifiedTransaction()  = default;
  UnverifiedTransaction(UnverifiedTransaction &&other) = default;
  UnverifiedTransaction& operator=(UnverifiedTransaction &&other) = default;
  
  UnverifiedTransaction(UnverifiedTransaction const &other) 
  {
    this->Copy(other);
  }  
  
  UnverifiedTransaction& operator=(UnverifiedTransaction const &other) 
  {
    this->Copy(other);
    return *this;    
  }

  

  bool operator<(UnverifiedTransaction const&other) const 
  {
    return digest() < other.digest();
  }
  
  
  using super_type::VERSION;  
  using super_type::hasher_type;
  using super_type::digest_type;
  //using super_type::;
  using super_type::groups;
  using super_type::summary;
  using super_type::data;
  using super_type::signature;  
  using super_type::contract_name;    
  using super_type::digest;
  
  MutableTransaction GetMutable() 
  {
    MutableTransaction ret;
    ret.Copy( *this );
    return ret;
  }
protected:
  using super_type::set_summary;
  using super_type::set_data;
  using super_type::set_signature;
  using super_type::set_contract_name;
  
  using super_type::UpdateDigest;
  using super_type::Verify;  

  using super_type::Copy;
  
  
  void Copy(UnverifiedTransaction const &tx) 
  {
    super_type::Copy(tx);
  }
  
  
  template <typename T>
  friend void Serialize(T &serializer, UnverifiedTransaction const &b);
  
  template <typename T>
  friend void Deserialize(T &serializer, UnverifiedTransaction &b);
};

class VerifiedTransaction : public UnverifiedTransaction
{
public:
  typedef UnverifiedTransaction super_type;  

  VerifiedTransaction()  = default;
  VerifiedTransaction(VerifiedTransaction &&other) = default;
  VerifiedTransaction& operator=(VerifiedTransaction &&other) = default;
  
  VerifiedTransaction(VerifiedTransaction const &other) {
    this->Copy(other);
  }

  VerifiedTransaction& operator=(VerifiedTransaction const &other) 
  {
    this->Copy(other);
    return *this;        
  }
  

  using super_type::GetMutable;

  static VerifiedTransaction Create(fetch::chain::MutableTransaction &&trans)
  {
    fetch::chain::MutableTransaction x;
    std::swap(x,trans);
    return VerifiedTransaction::Create(x);
  }
  
  
  static VerifiedTransaction Create(fetch::chain::MutableTransaction &trans)
  {
    VerifiedTransaction ret;
    ret.Finalise(trans);
    return ret;    
  }
  

  static VerifiedTransaction Create(UnverifiedTransaction &&trans)
  {
    UnverifiedTransaction x;
    std::swap(x,trans);
    return VerifiedTransaction::Create(x);
  }
  
  
  static VerifiedTransaction Create(UnverifiedTransaction &trans)
  {
    VerifiedTransaction ret;
    ret.Finalise(trans);
    return ret;    
  }
  


protected:
  void Copy(VerifiedTransaction const &tx) 
  {
    super_type::Copy(tx);
  }
  
  bool Finalise(fetch::chain::MutableTransaction &base) 
  {
    this->Copy(base);
    UpdateDigest();
    return Verify();
  }

  bool Finalise(fetch::chain::UnverifiedTransaction &base) 
  {
    this->Copy(base);
    UpdateDigest();
    return Verify();
  }
  
  using super_type::set_summary;
  using super_type::set_data;
  using super_type::set_signature;
  using super_type::set_contract_name;
  using super_type::Copy;
  using super_type::UpdateDigest;
  using super_type::Verify;  
  
  template <typename T>
  friend void Serialize(T &serializer, VerifiedTransaction const &b);
  
  template <typename T>
  friend void Deserialize(T &serializer, VerifiedTransaction &b);
};

typedef VerifiedTransaction Transaction;

} // namespace chain
} // namespace fetch

#endif
