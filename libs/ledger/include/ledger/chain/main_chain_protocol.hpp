#pragma once
#include "network/service/protocol.hpp"
#include "network/service/publication_feed.hpp"
#include "network/service/function.hpp"
#include "network/generics/subscriptions_container.hpp"
#include "network/generics/work_items_queue.hpp"
#include <vector>

namespace fetch {
namespace chain {

template <typename R>
class MainChainProtocol : public fetch::service::Protocol
                        , public fetch::service::HasPublicationFeed
{
public:
  using block_type            = chain::MainChain::block_type;
  using block_hash_type       = chain::MainChain::block_hash;
  using protocol_handler_type = service::protocol_handler_type;
  using thread_pool_type      = network::ThreadPool;
  using register_type         = R;
  using self_type             = MainChainProtocol<R>;

  enum
  {
    GET_HEADER         = 1,
    GET_HEAVIEST_CHAIN = 2,
    BLOCK_PUBLISH      = 3,
  };

  MainChainProtocol(protocol_handler_type const &p, register_type const &r,
                    thread_pool_type const &nm, chain::MainChain *chain)
      : Protocol(), protocol_(p), register_(r), thread_pool_(nm), chain_(chain), running_(false)
  {
    this->Expose(GET_HEADER, this, &self_type::GetHeader);
    this->Expose(GET_HEAVIEST_CHAIN, this, &self_type::GetHeaviestChain);

    this->RegisterFeed(BLOCK_PUBLISH, this);
    max_size_ = 100;
  }

  void Start()
  {
    fetch::logger.Debug("Starting syncronisation of blocks");
    if (running_) return;
    running_ = true;
    thread_pool_->Post([this]() { this->IdleUntilPeers(); });
  }

  void Stop() { running_ = false; }

  void PublishBlock(const chain::MainChain::block_type &blk)
  {
       LOG_STACK_TRACE_POINT;
   fetch::logger.Warn("MINED A BLOCK:" + blk.summarise());
    Publish(BLOCK_PUBLISH, blk);
  }

  void ConnectionDropped(fetch::network::TCPClient::handle_type connection_handle)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<mutex::Mutex> lock(mutex_);
    blockPublishSubscriptions_.ConnectionDropped(connection_handle);
  }
private:
  protocol_handler_type  protocol_;
  register_type          register_;
  thread_pool_type       thread_pool_;
  network::SubscriptionsContainer blockPublishSubscriptions_;

  /// Protocol logic
  /// @{


  void IdleUntilPeers()
  {
      LOG_STACK_TRACE_POINT;
    if (!running_) return;

    if (register_.number_of_services() == 0)
    {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); },
                         1000);  // TODO: Make time variable
    }
    else
    {
      thread_pool_->Post([this]() { this->FetchHeaviestFromPeers(); });
    }
  }

  void FetchHeaviestFromPeers()
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Fetching blocks from peer");

    if (!running_) return;

    uint32_t                      ms = max_size_;
    using service_map_type           = typename R::service_map_type;
    register_.WithServices([this, ms](service_map_type const &map) {
      for (auto const &p : map)
      {
        if (!running_)
        {
          return;
        }

        auto peer = p.second;
        auto ptr  = peer.lock();

        auto foo = new service::Function<void(chain::MainChain::block_type)>(
          [this](chain::MainChain::block_type block){
            this -> pending_blocks_.Add(block);
            this -> thread_pool_ -> Post([this]() { this -> AddPendingBlocks(); });
          }
        );
        blockPublishSubscriptions_.Subscribe(
          ptr,
          protocol_,
          BLOCK_PUBLISH,
          foo);

        auto prom = ptr->Call(protocol_, GET_HEAVIEST_CHAIN, ms);
        prom.Then([prom, ms, this](){
          std::vector<block_type> incoming;
          incoming.reserve(uint64_t(ms));
          prom.As(incoming);
          this -> pending_blocks_.Add(incoming.begin(), incoming.end());
          this -> thread_pool_ -> Post([this]() { this -> AddPendingBlocks(); });
        });
      }
    });

    if (running_)
    {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); }, 2000);
    }
  }

  void ForwardBlocks()
  {
    std::vector<block_type> work;
    if (forward_blocks_.Get(work, 16))
    {
       for(auto &block: work)
       {
         Publish(BLOCK_PUBLISH, block);
       }
    }
    if (forward_blocks_.Remaining())
    {
      this -> thread_pool_ -> Post([this]() { this -> ForwardBlocks(); });
    }
  }

  void AddPendingBlocks()
  {
    std::vector<block_type> work;

    if (pending_blocks_.Get(work, 16))
    {
      for(auto &block: work)
      {
        block.UpdateDigest();
        if (chain_->AddBlock(block))
        {
          forward_blocks_.Add(block);
          this -> thread_pool_ -> Post([this]() { this -> AddPendingBlocks(); });
          if (block.loose())
          {
            this -> thread_pool_ -> Post([this]() { this -> QueryLooseBlocks(); });
          }
        }
      }
    }
    if (pending_blocks_.Remaining())
    {
      this -> thread_pool_ -> Post([this]() { this -> AddPendingBlocks(); });
    }
    if (forward_blocks_.Remaining())
    {
      this -> thread_pool_ -> Post([this]() { this -> ForwardBlocks(); });
    }
    if (loose_blocks_.Remaining())
    {
      this -> thread_pool_ -> Post([this]() { this -> QueryLooseBlocks(); });
    }
  }

  void QueryLooseBlocks()
  {
    std::vector<block_type> work;
    if (loose_blocks_.Get(work, 16))
    {
    }
    if (loose_blocks_.Remaining())
    {
      this -> thread_pool_ -> Post([this]() { this -> ForwardBlocks(); });
    }
  }

  /// @}

  /// RPC
  /// @{
  std::pair<bool, block_type> GetHeader(block_hash_type const &hash)
  {
      LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("GetHeader starting work");
    block_type block;
    if (chain_->Get(hash, block))
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
      LOG_STACK_TRACE_POINT;
    std::vector<block_type> results;

    fetch::logger.Debug("GetHeaviestChain starting work ", maxsize);

    results = chain_->HeaviestChain(maxsize);

    fetch::logger.Debug("GetHeaviestChain returning ", results.size(), " of req ", maxsize);

    return results;
  }
  /// @}

  chain::MainChain *chain_;
  mutex::Mutex      mutex_{ __LINE__, __FILE__ };

  generics::WorkItemsQueue<block_type> pending_blocks_;
  generics::WorkItemsQueue<block_type> loose_blocks_;
  generics::WorkItemsQueue<block_type> forward_blocks_;

  std::atomic<bool>     running_;
  std::atomic<uint32_t> max_size_;
};

}  // namespace chain
}  // namespace fetch
