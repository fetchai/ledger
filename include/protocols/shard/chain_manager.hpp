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
  // Transaction defs
  typedef fetch::chain::Transaction transaction_type;
  typedef typename transaction_type::digest_type tx_digest_type;

  // Block defs  
  typedef fetch::chain::consensus::ProofOfWork proof_type;
  typedef BlockBody block_body_type;
  typedef typename proof_type::header_type block_header_type;
  typedef BlockMetaData block_meta_data_type;
  typedef fetch::chain::BasicBlock< block_body_type, proof_type, fetch::crypto::SHA256, block_meta_data_type > block_type;  
  
  typedef crypto::CallableFNV hasher_type;

  struct PartialChain 
  {
    block_header_type start;    
    block_header_type end;
    block_header_type next_missing;    
  };  


  
  ChainManager( TransactionManager &txmanager) :
    tx_manager_(txmanager) {}

  
  enum {
    ADD_NOTHING_TODO = 0,
    ADD_LOOSE_BLOCK = 1,
    ADD_CHAIN_END = 2    
  };

  
  bool AddBulkBlocks(std::vector< block_type > const &new_blocks ) 
  {
    bool ret = false;
//    block_type best_block = head_;
    
    for(auto block: new_blocks) {
      ret |=  (AddBlock( block ) != ADD_NOTHING_TODO );
    }
    // TODO: Attach block
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
    block.meta_data().loose_chain = true;    
    chains_[block.header()] = block;
//    tx_manager_.Apply( block );

    // TODO: Set next
    
    if((block.meta_data().total_work >= head_.meta_data().total_work)) {      
      head_ = block;
    }
    
    return  ADD_CHAIN_END ;
  }

  block_type const &head() const 
  {
    return head_;    
  }

  std::map< block_header_type, block_type > const &chains() const
  {
    return chains_;
  }

  std::map< block_header_type, block_type > &chains() 
  {
    return chains_;
  }
  
  
  std::map< uint64_t, PartialChain > const &loose_chains() const
  {
    return loose_chains_;
  }

  bool VerifyState() {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;    
    auto block = head_;
    
    std::vector< tx_digest_type > transactions;
    while(block.body().previous_hash != "genesis") {
      transactions.push_back( block.body().transaction_hash );
      
      block = chains_[block.body().previous_hash];
    }

    std::reverse(transactions.begin(), transactions.end());
    return tx_manager_.VerifyAppliedList(transactions);
  }   

  std::vector< block_type > const &latest_blocks() const {
    return latest_blocks_; 
  }

  std::size_t size() const 
  {
    return chains_.size();
  }
  
private:
  TransactionManager &tx_manager_;

  std::map< block_header_type, block_type > chains_;
  std::map< uint64_t, PartialChain > loose_chains_;
  uint64_t loose_chain_counter_ = 0;  
  std::map< block_header_type, std::vector< uint64_t > > loose_chain_bottoms_;
  std::map< block_header_type, uint64_t > loose_chain_tops_;  
  
  std::vector< block_header_type > heads_;  
  block_type head_;


  std::vector< block_type > latest_blocks_;
  
};


};
};


#endif
