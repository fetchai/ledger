#ifndef PROTOCOLS_SHARD_CHAIN_MANAGER_HPP
#define PROTOCOLS_SHARD_CHAIN_MANAGER_HPP

#include"crypto/fnv.hpp"
#include"chain/transaction.hpp"
#include"assert.hpp"
#include"protocols/shard/transaction_manager.hpp"

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
  typedef BlockMetaData block_meta_data_type;
  typedef fetch::chain::BasicBlock< block_body_type, proof_type, fetch::crypto::SHA256, block_meta_data_type > block_type;  
  
  typedef std::unordered_map< block_header_type, block_type, hasher_type > chain_map_type;
  
  
  ChainManager( )  {}
  
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
    fetch::logger.Debug("Entering ", __FUNCTION_NAME__);    
    
    // Only record blocks that are new
    if( chains_.find( block.header() ) != chains_.end() ) {
      fetch::logger.Debug("Nothing todo");
      return ADD_NOTHING_TODO;
    }

    TODO("Trim latest blocks");
    latest_blocks_.push_back(block);
    
    block_header_type header = block.header();
//    block.meta_data().loose_chain = true;    
    chains_[block.header()] = block;

    // TODO: Set next
    
    if((block.meta_data().total_work >= head_.meta_data().total_work)) {
      head_ = block;
    }
    
    return ADD_CHAIN_END;
  }

  void UpdateAppliedList() 
  {
    std::vector< byte_array::ByteArray > applied_list_;
    auto block = head_;
    bool broken = false;    
    
    while(block.body().previous_hash != "genesis") {
      applied_list_.push_back( block.body().transaction_hash );
      if(chains_.find( block.body().previous_hash ) == chains_.end() ) {
        broken = true;
        break;        
      }
      
      block = chains_[block.body().previous_hash];
    }

    if(broken) {
      TODO("Figure out what to do with a broken chain");
    }       
  }
  

  block_type const &head() const 
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
  
private:
  chain_map_type chains_;

  block_type head_;

  std::vector< block_type > latest_blocks_;  
};


};
};


#endif
