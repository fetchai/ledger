#ifndef PROTOCOLS_MAIN_CHAIN_PROTOCOL_HPP
#define PROTOCOLS_MAIN_CHAIN_PROTOCOL_HPP
#include "network/service/protocol.hpp"
#include<vector>

namespace fetch
{
namespace chain
{

template< typename R >
class MainChainProtocol : public fetch::service::Protocol
{
public:
  using block_type = chain::MainChain::block_type;
  using block_hash_type = chain::MainChain::block_hash;
  using protocol_handler_type = service::protocol_handler_type;  
  using thread_pool_type = network::ThreadPool;
  using register_type = R;
  using self_type = MainChainProtocol< R >;
  
  enum
  {
    GET_HEADER         = 1,
    GET_HEAVIEST_CHAIN = 2,
  };
  
  MainChainProtocol(protocol_handler_type const &p, register_type const& r, thread_pool_type const&nm, chain::MainChain *node)
    : Protocol(),
      protocol_(p),
      register_(r),
      thread_pool_(nm),
      chain_(node),
      running_(false) 
  {
    this->Expose(GET_HEADER, this,  &self_type::GetHeader);
    this->Expose(GET_HEAVIEST_CHAIN, this,  &self_type::GetHeaviestChain);
    max_size_ = 100;

   }
  
  void Start() 
  {
    fetch::logger.Debug("Starting syncronisation of blocks");    
    if(running_) return;    
    running_ = true;
    thread_pool_->Post([this]() { this->IdleUntilPeers(); } );
  }

  void Stop() 
  {
    running_ = false;
  }
  

private:
  protocol_handler_type protocol_;
  register_type register_;
  thread_pool_type thread_pool_;


  
  /// Protocol logic
  /// @{

  void IdleUntilPeers() 
  {
    if(!running_) return;
    
    if(register_.number_of_services() == 0 ) {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); }, 1000 ); // TODO: Make time variable
    } else {          
      thread_pool_->Post([this]() { this->FetchHeaviestFromPeers(); } );
    }
  }
  

  
  void FetchHeaviestFromPeers() 
  {
    fetch::logger.Debug("Fetching blocks from peer");
    
    if(!running_) return;
    
    std::lock_guard< mutex::Mutex > lock( block_list_mutex_);
    uint32_t ms = max_size_;    
    using service_map_type = typename R::service_map_type;
    register_.WithServices([this,ms](service_map_type const &map) {
        for(auto const &p: map)
        {
          if(!running_) return;
          
          auto peer = p.second;
          auto ptr = peer.lock();
          block_list_promises_.push_back( ptr->Call(protocol_, GET_HEAVIEST_CHAIN, ms ) );

        }
      });

    if(running_) {
      thread_pool_->Post([this]() { this->RealisePromises(); } );
    }
    
  }
  

  void RealisePromises() 
  {
    if(!running_) return;    
    std::lock_guard< mutex::Mutex > lock( block_list_mutex_);
    incoming_objects_.reserve(uint64_t(max_size_));
    
    for(auto &p : block_list_promises_)
    {
      
      if(!running_) return;
      
      incoming_objects_.clear();
      if(!p.Wait(100, false)) {
        continue;
      }
      
      p.template As< std::vector< block_type > >( incoming_objects_ );

      if(!running_) return;
      std::lock_guard< mutex::Mutex > lock(mutex_);

      bool loose = false;
      byte_array::ByteArray blockId;
      
      byte_array::ByteArray prevHash;
      for(auto &block : incoming_objects_)
      {
        block.UpdateDigest();
        chain_->AddBlock(block);
        prevHash = block.prev();        
        loose = block.loose();

      }
      if (loose)
      {
        fetch::logger.Warn("Loose block");
        TODO("Make list with missing blocks: ", prevHash);
      }      
      
    }

    block_list_promises_.clear();
    if(running_) {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); }, 5000 );  /// TODO: Set from parameter
    }
  }
  /// @}

  
  /// RPC
  /// @{
  std::pair<bool, block_type> GetHeader(block_hash_type const &hash)
  {
    fetch::logger.Debug("GetHeader starting work");
    block_type block;
    if (chain_ -> Get(hash, block))
    {
      fetch::logger.Debug("GetHeader done");
      return std::make_pair(true, block);
    }
    else
    {
      fetch::logger.Debug("GetHeader not found");
      return std::make_pair(false, block);
    }
  }

  std::vector<block_type> GetHeaviestChain(uint32_t const &maxsize)
  {
    std::vector<block_type> results;

    fetch::logger.Debug("GetHeaviestChain starting work ",  maxsize);
    auto currentHash = chain_ -> HeaviestBlock().hash();

    while(results.size() < maxsize)
    {
      block_type block;
      if (chain_ -> Get(currentHash, block))
      {
        results.push_back(block);
        currentHash = block.body().previous_hash;
      }
      else
      {
        break;
      }
    }

    fetch::logger.Debug("GetHeaviestChain returning ", results.size(), " of req ", maxsize);

    return results;
  }
  /// @}
 

  chain::MainChain *chain_;
  mutex::Mutex mutex_;  

  mutable mutex::Mutex block_list_mutex_;
  std::vector< service::Promise > block_list_promises_;
  std::vector< block_type > incoming_objects_;

  
  std::atomic< bool > running_ ;
  std::atomic< uint32_t > max_size_ ;
  
};

}
}

#endif //__MAIN_CHAIN_PROTOCOL_HPP
