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
    tx_manager_(txmanager) {
    
  }

  
  enum {
    ADD_NOTHING_TODO = 0,
    ADD_LOOSE_BLOCK = 1,
    ADD_CHAIN_END = 2    
  };

  uint32_t AddBlock(block_type &block ) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    fetch::logger.Debug("Entering ", __FUNCTION_NAME__);    
    
    // Only record blocks that are new
    if( chains_.find( block.header() ) != chains_.end() ) {
      fetch::logger.Debug("Nothing todo");    
      return ADD_NOTHING_TODO;
    }
    
    assert( chains_.find( block.header() ) == chains_.end() );
    block_header_type header = block.header();
    block.meta_data().loose_chain = true;    
    chains_[block.header()] = block;
    

    // Check if block is adding to a loose chain.
    bool was_loose = false;    
    if(loose_chain_tops_.find( block.body().previous_hash ) != loose_chain_tops_.end() )
    {
      /*                                 
       * Main chain
       * with path to genesis             
       * ┌──────┐                        
       * │      │                        
       * │      │   Missing block        
       * └──────┘   ┌ ─ ─ ─              
       *     │             │             
       *     │      │                    
       *     ▼       ─ ─ ─ ┘             
       * ┌──────┐       │                
       * │      │       │                
       * │      │       ▼ Loose chains   
       * └──────┘   ┌──────┐ with no path            
       *     │      │      │ to genesis
       *     │      │      │             
       *     ▼      └──────┘             
       * ┌──────┐       │                
       * │      │       └──┐             
       */
      was_loose = true;
      
      std::size_t i = loose_chain_tops_[block.body().previous_hash];
      auto it = loose_chain_tops_.find( block.body().previous_hash ) ;
      loose_chain_tops_.erase( it );

      assert( i < loose_chains_.size() );      
      auto &pc = loose_chains_[i];

      pc.start = header;
      loose_chain_tops_[header] = i;      
    }


    if(loose_chain_bottoms_.find( header ) != loose_chain_bottoms_.end() )
    {
      /*         
       * Chains with
       * path to         
       * genesis         Loose chains        
       * ┌──────┐       │       │      │      
       * │      │       │       │      │      
       * │      │       ▼       └──────┘      
       * └──────┘   ┌──────┐        │         
       *     │      │      │        ▼         
       *     │      │      │    ┌──────┐      
       *     ▼      └──────┘    │      │      
       * ┌──────┐       │       │      │      
       * │      │       └──┐    └──────┘      
       * │      │          │        │         
       * └──────┘          ▼        │         
       *     │         ┌ ─ ─ ─      │         
       *     │                │     │         
       *     ▼         │       ◀────┘         
       * ┌──────┐       ─ ─ ─ ┘               
       * │      │                            
       * │      │      Missing block  
       * └──────┘                             
       */
      was_loose = true;
      std::vector< uint64_t > lchains = loose_chain_bottoms_[header];
      auto it = loose_chain_bottoms_.find( header ) ;
      loose_chain_bottoms_.erase( it );

      for(auto &id: lchains) {
        auto &pc = loose_chains_[id];
        pc.end = header;
        pc.next_missing = block.body().previous_hash;        
      }

      if( loose_chain_bottoms_.find( block.body().previous_hash ) == loose_chain_bottoms_.end() )
      {
        // Even though the chains merge, the remain many separate chains
        loose_chain_bottoms_[block.body().previous_hash] = lchains;
      }
      else
      {
        
        auto &l = loose_chain_bottoms_[block.body().previous_hash];
        for(auto &id : lchains)
        {
          l.push_back(id);          
        }        
      }
            

      /* Chain with 
       * path 
       * to genesis    Loose chains
       *
       * ┌──────┐       │       │      │      
       * │      │       └──┐    └──────┘      
       * │      │          │        │         
       * └──────┘          ▼        │         
       *     │         ┌ ─ ─ ─      │         
       *     │                │     │         
       *     ▼         │       ◀────┘         
       * ┌──────┐       ─ ─ ─ ┘               
       * │      │          │                  
       * │      │◀─────────┘   Missing block  
       * └──────┘                             
       *     │
       *
       * Checking if path to genesis block exists
       */
      if( chains_.find(block.body().previous_hash) != chains_.end() ) {
        auto &next = chains_[block.body().previous_hash];

        // If so, the block is final and we are ready to move the loose chain to the
        // main tree
        if(next.meta_data().loose_chain == false) {
          auto &l = loose_chain_bottoms_[block.body().previous_hash];
          for(auto &id: l)
          {
            auto &pc = loose_chains_[id];
            block_header_type h = pc.start;    
            block_type b = chains_[header];

            AttachBlock(h,b);
            
            auto tit = loose_chain_tops_.find( h );            
            if(tit != loose_chain_tops_.end())
            {
              loose_chain_tops_.erase( tit );              
            }
            
            loose_chains_.erase( loose_chains_.find( id ) );            
          }
          

          auto it = loose_chain_bottoms_.find(block.body().previous_hash);
          loose_chain_bottoms_.erase(it);          
        }
      }
    }

    return was_loose? ADD_LOOSE_BLOCK : ADD_CHAIN_END ;
  }

  
  void SwitchBranch(block_type new_head, block_type old_head)
  {
    // Rolling back

    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    head_ = new_head;
    
//    block_mutex_.lock();
    if( new_head.meta_data().block_number == BlockMetaData::UNDEFINED) {
      fetch::logger.Error("Block number is undefined!");
      
      return;      
    }
    
    if(new_head.header() == old_head.header()) {
      fetch::logger.Highlight("No change.");
//      block_mutex_.unlock();      
      return;      
    }

    std::vector< tx_digest_type > used_transactions;
    

    if(new_head.body().previous_hash == old_head.header())
    {

      used_transactions.push_back(new_head.body().transaction_hash);
    }
    else
    {      
      fetch::logger.Highlight("Rolling back");
      std::size_t roll_back_count = 0;          
      while(new_head.meta_data().block_number > old_head.meta_data().block_number)
      {
        used_transactions.push_back(new_head.body().transaction_hash);
        new_head = chains_[new_head.body().previous_hash];
        fetch::logger.Debug("Block nr comp 1: ", new_head.meta_data().block_number," ", old_head.meta_data().block_number, " ", BlockMetaData::UNDEFINED);
      }
      
      while(new_head.meta_data().block_number < old_head.meta_data().block_number)
      {
        ++roll_back_count;
        old_head = chains_[old_head.body().previous_hash];
        fetch::logger.Debug("Block nr comp 2: ", new_head.meta_data().block_number," ",  old_head.meta_data().block_number);
      } 
      
      while(new_head.header() != old_head.header() )
      {
        fetch::logger.Debug(byte_array::ToBase64( new_head.header() ), " vs ",  byte_array::ToBase64( old_head.header()) );
        used_transactions.push_back(new_head.body().transaction_hash);
        ++roll_back_count;        
        new_head = chains_[new_head.body().previous_hash];
        old_head = chains_[old_head.body().previous_hash];
      }
      
      tx_manager_.RollBack( roll_back_count );      
    }

    
    // Rolling forth
    
    while( !used_transactions.empty() )
    {      
      auto tx = used_transactions.back();
      used_transactions.pop_back();
      tx_manager_.Apply( tx );
    }
        
  }

  
  void AttachBlock(block_header_type header, block_type &block) 
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    
    // Tracing the way back to a chain that leads to genesis
    // TODO, FIXME: Suceptible to attack: Place a block that creates a loop.
    std::stack< block_header_type > visited_blocks;    
    while(chains_.find(header) != chains_.end()) {      
      auto b = chains_[header];
      visited_blocks.push( header );

      /*
        TODO: We can do performance optimisation here.
        This is wrong as the code does not support half chains at the moment
      if(block.meta_data().block_number != block_meta_data_type::UNDEFINED)
      {        
        break;
      }
      */
      
      header = b.body().previous_hash;
    }

    // By design visited blocks must contain the latest submitted block.
    assert( visited_blocks.size() > 0 );
    block_header_type earliest_header = visited_blocks.top();
    block_type earliest_block = chains_[earliest_header];
    
    
    // Computing the total work that went into the chain.
    if(block.body().transaction_hash == "genesis") {
      fetch::logger.Debug( "Adding genesis" );
      
      block.meta_data().loose_chain = false;      
      chains_[block.header()] = block;
      head_ = block;
      
      return;
    } else if( earliest_block.body().transaction_hash != "genesis" ) {
      // Creating loose chain - we are sure that it does not add to existing
      // loose chains beause we checked that earlier.

      PartialChain pc;
      pc.start = block.header();
      pc.end = earliest_block.header();      
      pc.next_missing = earliest_block.body().previous_hash;

      
      std::size_t i = loose_chain_counter_;
      loose_chains_[i] = pc;      
      loose_chain_tops_[ pc.start] = i;
      ++loose_chain_counter_;

      
      if(loose_chain_bottoms_.find(pc.next_missing) == loose_chain_bottoms_.end() ) {
        loose_chain_bottoms_[ pc.next_missing ] = std::vector< uint64_t >();       
      }
      
      loose_chain_bottoms_[ pc.next_missing ].push_back(i);      
           
    } else {
      fetch::logger.Debug( "Found root: ", header );      

      block.meta_data().loose_chain = false;
      chains_[block.header()] = block;
      
      header = visited_blocks.top();
      auto b1 = chains_[header];
      visited_blocks.pop();

      while(!visited_blocks.empty()) {
        header = visited_blocks.top();
        auto b2 = chains_[header];

        auto &p = b2.proof();
        p();
        double work = fetch::math::Log( p.digest() );
        
        b2.meta_data() = b1.meta_data();
        ++b2.meta_data().block_number;

        // TODO: Check the correct way to compute the strongest chain - looks wrong
        b2.meta_data().total_work += work;
        b2.meta_data().loose_chain = false;        
        chains_[header] = b2;

        b1 = b2;
        visited_blocks.pop();

      }

      block = chains_[header];
    }

    if(block.meta_data().total_work > head_.meta_data().total_work) { 
      
      this->Commit(block);       
    }
  }

  void Commit(block_type const &block) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    fetch::logger.Debug("Entering ", __FUNCTION_NAME__);
    
    // We only commit if there actually is a new block
    if( block.meta_data().block_number > 0 ) {

      SwitchBranch(block, head_);      
      
      std::cout << "Applying block: " << head_.meta_data().block_number << " " <<  head_.meta_data().total_work <<  std::endl;
      std::cout << "  <- " << fetch::byte_array::ToBase64( head_.body().previous_hash ) << std::endl;
      std::cout << "   = " << fetch::byte_array::ToBase64( head_.header() ) << std::endl;
      std::cout << "    (" << fetch::byte_array::ToBase64( head_.body().transaction_hash ) << ")" << std::endl;


      fetch::logger.Info("Synced to: ", fetch::byte_array::ToBase64( head_.header() ));
    }

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

  
private:
  TransactionManager &tx_manager_;

  std::map< block_header_type, block_type > chains_;
  std::map< uint64_t, PartialChain > loose_chains_;
  uint64_t loose_chain_counter_ = 0;  
  std::map< block_header_type, std::vector< uint64_t > > loose_chain_bottoms_;
  std::map< block_header_type, uint64_t > loose_chain_tops_;  
  
  std::vector< block_header_type > heads_;  
  block_type head_;
  
};


};
};


#endif
