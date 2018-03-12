#ifndef PROTOCOLS_SHARD_TRANSACTION_MANAGER_HPP
#define PROTOCOLS_SHARD_TRANSACTION_MANAGER_HPP

#include"crypto/fnv.hpp"
#include"chain/transaction.hpp"
#include"assert.hpp"

#include<unordered_set>

namespace fetch
{
namespace protocols
{
/**

 **/
class TransactionManager 
{
public:
  typedef crypto::CallableFNV hasher_type;
  
  // Transaction defs
  typedef fetch::chain::Transaction transaction_type;
  typedef typename transaction_type::digest_type tx_digest_type;
  
  
  void Apply(tx_digest_type const &tx)
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );
    
    // TODO
    // Particular important detail: We allow for not known transactions to be applied to the chain
    // This potentially constitutes an attack vector that could lay down the network.
    // I.e. what happens to the chain when a non-existing transaction is included into the chain?
    if(known_transactions_.find( tx  ) == known_transactions_.end())
    {
      fetch::logger.Error("Trying to apply transaction that is not known: ", byte_array::ToBase64(tx));
      TODO("mark as missing");
    } else {
      
      auto it = unapplied_.find( tx ) ;      
      if(it == unapplied_.end() ) {
        fetch::logger.Warn("Cannot apply applied transaction: ", byte_array::ToBase64(tx));

        bool was_applied = false;
        

        for(auto &a : applied_) {
          if(a == tx)
          {
            was_applied = true;
            fetch::logger.Highlight(" >> ", byte_array::ToBase64(a));
          }
          else
          {
            fetch::logger.Debug(" >> ", byte_array::ToBase64(a));
          }
          
        }

        if(was_applied) {
          
          TODO_FAIL("Transaction was already applied");
        } else  {
          
          TODO_FAIL("Transaction was not applied");
        }
        

      }
            unapplied_.erase(it);
      
    }

    bool fail = false;
    
    for(auto &a : applied_) {
      if(a == tx) fail = true;
    }
    if(fail) {
      fetch::logger.Error("TX Exists 1: ", byte_array::ToBase64(tx));
    }
    
      
    applied_.push_back( tx );
  }
  
  bool AddTransaction(transaction_type const &tx)
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );    
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;    
    if(known_transactions_.find( tx.digest()  ) != known_transactions_.end())
    {
      return false;
    }
    
    TODO("Check if transaction belongs to shard");    
    
    transactions_[ tx.digest() ] = tx;
    known_transactions_.insert( tx.digest() );    
    unapplied_.insert( tx.digest() );
    
    return true;    
  }


  void RollBack(std::size_t n) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );    
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;    
    while( (n != 0) ) {
      --n;
      detailed_assert( applied_.size() != 0 );
      
      auto tx = applied_.back();
      applied_.pop_back();
      
      unapplied_.insert( tx );
      
    }
    
  }

  bool has_unapplied() const 
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );        
    return unapplied_.size() > 0;
  }

  tx_digest_type NextDigest() 
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );    
    detailed_assert( unapplied_.size() > 0 );
    auto it = unapplied_.begin();
    return *it;    
  }
  
  transaction_type Next() 
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );    
    detailed_assert( unapplied_.size() != 0 );
    auto it = unapplied_.begin();
    return  transactions_[ *it ];    
  }

  std::size_t unapplied_count() const 
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );    
    return unapplied_.size();
  }

  std::size_t applied_count() const 
  {    
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );
    return applied_.size();
  }

  std::size_t size() const 
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );        
    return known_transactions_.size();
  }
  
  
  tx_digest_type top() const
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );
    return applied_.back();
  }

  bool VerifyAppliedList(std::vector< tx_digest_type > const &ref) {
    using namespace fetch::byte_array;
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );
    bool ret = true;
    
    if(ref.size() != applied_.size()) {
      fetch::logger.Warn("Sizes mismatch");
      ret =  false;
    }
    
    for(std::size_t i=0; i < ref.size(); ++i) {
      if(ref[i] != applied_[i]) {
        fetch::logger.Warn("Transaction mismatch at ", i, ": ");
        fetch::logger.Warn( i," ", ToBase64( ref[i] ), " <> ", ToBase64( applied_[i] ));
        ret =  false;
      }
    }
    if(!ret) {
      for(std::size_t i=0; i < ref.size(); ++i) {
        fetch::logger.Debug( i,") ", ToBase64( ref[i] ), " == ", ToBase64( applied_[i] ));
      }
    }
    
    return ret;
  }
    
private:
  std::unordered_set< tx_digest_type, hasher_type > unapplied_;  
  std::unordered_set< tx_digest_type, hasher_type > known_transactions_;  
  std::vector< tx_digest_type > applied_;  
  
  std::map< tx_digest_type, transaction_type > transactions_;

  mutable fetch::mutex::Mutex mutex_;
  
};


};
};


#endif
