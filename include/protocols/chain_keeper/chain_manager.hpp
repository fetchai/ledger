#ifndef PROTOCOLS_CHAIN_KEEPER_CHAIN_MANAGER_HPP
#define PROTOCOLS_CHAIN_KEEPER_CHAIN_MANAGER_HPP

#include"crypto/fnv.hpp"
#include"chain/transaction.hpp"
#include"assert.hpp"
#include"protocols/chain_keeper/transaction_manager.hpp"

#include<unordered_set>
#include<stack>
#include<map>
#include<vector>

namespace fetch
{
namespace protocols
{

class ChainManager 
{
public:
  typedef crypto::CallableFNV hasher_type;
  
  // Transaction defs
  typedef fetch::chain::Transaction transaction_type;
  typedef typename transaction_type::digest_type tx_digest_type;

  // Block defs  
  typedef fetch::chain::consensus::ProofOfWork proof_type;
  typedef BlockBody block_body_type;
  typedef typename proof_type::header_type block_header_type;
  typedef fetch::chain::BasicBlock< block_body_type, proof_type, fetch::crypto::SHA256 > block_type;  
  typedef std::shared_ptr< block_type > shared_block_type;
  
  typedef std::unordered_map< block_header_type, shared_block_type, hasher_type > chain_map_type;
  
  
  ChainManager(TransactionManager &tx_manager) :
    tx_manager_(tx_manager) { group_ = 0; }
  
  enum {
    ADD_NOTHING_TODO = 0,
    ADD_CHAIN_END = 2    
  };

  
  bool AddBulkBlocks(std::vector< block_type > const &new_blocks ) 
  {
    bool ret = false;
    for(auto block: new_blocks) {
      ret |=  (AddBlock( block ) != ADD_NOTHING_TODO );
    }
    return ret;    
  }
  

  uint32_t AddBlock(block_type &block ) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    //    fetch::logger.Debug("Entering ", __FUNCTION_NAME__);    
    
    // Only record blocks that are new
    if( chains_.find( block.header() ) != chains_.end() ) {
      //      fetch::logger.Debug("Nothing todo");
      return ADD_NOTHING_TODO;
    }

    TODO("Trim latest blocks");
    latest_blocks_.push_back(block);
    
    block_header_type header = block.header();

    chain_map_type::iterator pit;
    bool found = false;
    
    for(auto &hash: block.body().previous_hashes ) {
      pit = chains_.find( hash );
      if( pit != chains_.end() ) {
        found = true;        
        break;
      }
      
      
    }
    

    if( pit != chains_.end() ) {
      block.add_previous( group_, pit->second );
      block.set_is_loose( pit->second->is_loose() );
    } else {
      // First block added is always genesis and by definition not loose
      block.set_is_loose( chains_.size() != 0 );
    }

    auto shared_block = std::make_shared< block_type>( block );
    chains_[block.header()] = shared_block;

    // TODO: Set next
    if(block.is_loose()) {
      fetch::logger.Debug("Found loose block");
    } else if(! head_ ) {
      head_ = shared_block;
      tx_manager_.UpdateApplied( head_ );      
    } else if((block.total_weight() >= head_->total_weight())) {
      head_ = shared_block;
      tx_manager_.UpdateApplied( head_ );
    }

    return ADD_CHAIN_END;
  }

  shared_block_type const &head() const 
  {
    return head_;    
  }

  chain_map_type const &chains() const
  {
    return chains_;
  }

  chain_map_type &chains() 
  {
    return chains_;
  }
  
  
  std::vector< block_type > const &latest_blocks() const {
    return latest_blocks_; 
  }

  std::size_t size() const 
  {
    return chains_.size();
  }
  
  void set_group(uint32_t g) 
  {
    group_ = g;
  }
  
  
private:
  TransactionManager &tx_manager_;
  std::atomic< uint32_t > group_ ;    
  chain_map_type chains_;

  shared_block_type head_;

  std::vector< block_type > latest_blocks_;  
};


};
};


#endif
