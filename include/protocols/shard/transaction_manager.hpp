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

  typedef fetch::chain::consensus::ProofOfWork proof_type;
  typedef BlockBody block_body_type;
  typedef typename proof_type::header_type block_header_type;
  typedef BlockMetaData block_meta_data_type;
  typedef fetch::chain::BasicBlock< block_body_type, proof_type, fetch::crypto::SHA256, block_meta_data_type > block_type;  
  typedef std::shared_ptr< block_type > shared_block_type;
  
  bool AddBulkTransactions(std::unordered_map< tx_digest_type, transaction_type, hasher_type > const &new_txs ) 
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    bool ret = false;    
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );
    for(auto const& t: new_txs) {
      if(known_transactions_.find( t.first  ) == known_transactions_.end())
      {
        auto const &tx = t.second;        
        ret = true;
        RegisterTransaction(tx);        
      }
    
    }
    
    
    return ret;
  }
  
  bool AddTransaction(transaction_type const &tx)
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;        
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );    
   
    if(known_transactions_.find( tx.digest()  ) != known_transactions_.end())
    {
      return false;
    }
    RegisterTransaction( tx );        
    return true;    
  }

  void UpdateApplied(shared_block_type shared_block) {
    std::vector< tx_digest_type > new_applied;
    fetch::logger.Highlight("Applying block");
    std::cout << "Was here?" << std::endl;
    do {
      new_applied.push_back( shared_block->body().transaction_hash );

      shared_block = shared_block->previous();
    } while( shared_block );

    for(auto &a : applied_) {
      unapplied_.insert( a );
    }

    fetch::logger.Highlight("ADDING ", new_applied.size(), " Transactoins");
    applied_.clear();
    while( !new_applied.empty() ) {
      auto &a = new_applied.back();
      new_applied.pop_back();
      applied_.push_back(a);
      auto it = unapplied_.find(a);
      if(it == unapplied_.end() ) {
        fetch::logger.Debug("Transaction not known!!!!!!");
        continue;
      }
      unapplied_.erase(a);
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
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;    
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

  std::vector< transaction_type > LastTransactions() const
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );    
    return last_transactions_;    
  }
  
private:

  void RegisterTransaction(transaction_type const& tx ) 
  {
    
    TODO("Check if transaction belongs to shard");    
    
    last_transactions_.push_back(tx);
    transactions_[ tx.digest() ] = tx;
    known_transactions_.insert( tx.digest() );    
    unapplied_.insert( tx.digest() );
    fetch::logger.Highlight("========================================= >>>>>>>>>>>>>>>>>>> ", known_transactions_.size());

    TODO("Trim last transactions");

  }
  
  std::vector< transaction_type > last_transactions_;
  
  std::unordered_set< tx_digest_type, hasher_type > unapplied_;  
  std::unordered_set< tx_digest_type, hasher_type > known_transactions_;  
  std::vector< tx_digest_type > applied_;  
  
  std::unordered_map< tx_digest_type, transaction_type, hasher_type > transactions_;

  mutable fetch::mutex::Mutex mutex_;
  
};


};
};


#endif
