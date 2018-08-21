#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/main_chain_details.hpp"
#include "network/details/thread_pool.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "network/generics/subscriptions_container.hpp"
#include "network/generics/work_items_queue.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/function.hpp"
#include "network/service/protocol.hpp"
#include "network/service/publication_feed.hpp"

#include <utility>
#include <vector>
#include <typeinfo>
namespace fetch {
namespace chain {

template <typename R>
class MainChainProtocol : public fetch::service::Protocol, public fetch::service::HasPublicationFeed
{
public:
  using BlockType                 = chain::MainChain::BlockType;
  using block_type                = chain::MainChain::BlockType;
  using block_hash_type           = chain::MainChain::BlockHash;
  using protocol_number_type      = service::protocol_handler_type;
  using thread_pool_type          = network::ThreadPool;
  using register_type             = R;
  using self_type                 = MainChainProtocol<R>;
  using connectivity_details_type = MainChainDetails;
  using client_register_type      = fetch::network::ConnectionRegister<connectivity_details_type>;
  using client_handle_type        = client_register_type::connection_handle_type;
  using feed_handler_type         = service::feed_handler_type;

  static constexpr char const *LOGGING_NAME = "MainChainProtocol";

  enum
  {
    GET_HEADER         = 1,
    GET_HEAVIEST_CHAIN = 2,
    BLOCK_PUBLISH      = 3,
    GET_B              = 4,
  };

  MainChainProtocol(protocol_number_type const &p, register_type r, thread_pool_type nm,
                    const std::string &identifier, chain::MainChain *node)
    : Protocol()
    , protocol_(p)
    , register_(r)
    , thread_pool_(nm)
    , chain_(node)
    , running_(false)
    , identifier_(identifier)
  {
    this->Expose(GET_HEADER, this, &self_type::GetHeader);
    this->Expose(GET_HEAVIEST_CHAIN, this, &self_type::GetHeaviestChain);
    this->Expose(GET_B, this, &self_type::GetB);

    this->RegisterFeed(BLOCK_PUBLISH, this);
    max_size_ = 100;
  }

  void Start()
  {
    FETCH_LOG_DEBUG(LOGGING_NAME,"Starting syncronisation of blocks");
    if (running_) return;
    running_ = true;
    thread_pool_->Post([this]() { this->IdleUntilPeers(); });
  }

  void Stop() { running_ = false; }

  void PublishBlock(BlockType const &blk)
  {
    LOG_STACK_TRACE_POINT;
    FETCH_LOG_WARN(LOGGING_NAME,"MINED A BLOCK:" + blk.summarise());
    Publish(BLOCK_PUBLISH, blk);
  }

  void ConnectionDropped(fetch::network::TCPClient::handle_type connection_handle)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<mutex::Mutex> lock(mutex_);
    blockPublishSubscriptions_.ConnectionDropped(connection_handle);
  }

  std::vector<std::string> GetCurrentSubscriptions()
  {
    return blockPublishSubscriptions_.GetAllSubscriptions(protocol_, BLOCK_PUBLISH);
  }

  void AssociateName(const std::string &name, client_handle_type connection_handle,
                     protocol_number_type proto = 0, feed_handler_type verb = 0)
  {
    blockPublishSubscriptions_.AssociateName(name, connection_handle, proto, verb);
  }

  const std::string &GetIdentity() { return identifier_; }

private:
  protocol_number_type            protocol_;
  register_type                   register_;
  thread_pool_type                thread_pool_;
  network::SubscriptionsContainer blockPublishSubscriptions_;

  /// Protocol logic
  /// @{

  void IdleUntilPeers()
  {
    if (!running_) return;

    if (register_.number_of_services() == 0)
    {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); },
                         1000);  // TODO(issue 7): Make time variable
    }
    else
    {
      thread_pool_->Post([this]() { this->FetchHeaviestFromPeers(); });
    }
  }

  void FetchHeaviestFromPeers()
  {
    LOG_STACK_TRACE_POINT;
    FETCH_LOG_DEBUG(LOGGING_NAME,"Fetching blocks from peer");

    if (!running_) return;

    uint32_t ms            = max_size_;
    using service_map_type = typename R::service_map_type;
    using service_map_items          = typename service_map_type::value_type;
    register_.VisitServiceClients([this, ms](service_map_items const &p)
    {
      LOG_STACK_TRACE_POINT;
      if (!running_)
      {
        return;
      }

      auto service_client = p.second.lock();
      auto details = register_.GetDetails(service_client -> handle());
      if ((!details -> is_outgoing) || (!details -> is_peer))
      {
        //std::cout << std::string(byte_array::ToBase64(details.identity.identifier())) << std::endl;
        // std::endl;
        return;
        //if (!details -> IsAnyMainChain())
      }

      auto name = details->GetOwnerIdentityString();

      auto subscription_handler_cb = [this](block_type block)
        {
          this -> pending_blocks_.Add(block);
          this -> thread_pool_ -> Post([this]() { this -> AddPendingBlocks(); });
        };

      auto subscription_handler_function = new service::Function<void(block_type)>(subscription_handler_cb);

      {
        LOG_STACK_TRACE_POINT;
        blockPublishSubscriptions_.Subscribe(service_client, protocol_, BLOCK_PUBLISH,
                                             name, // TODO(kll) make a connection name here.
                                             subscription_handler_function);
      }

      // TODO(EJF): ?????
      if (0)
      {
        LOG_STACK_TRACE_POINT;
        auto prom = service_client->Call(protocol_, GET_HEAVIEST_CHAIN, ms);
        prom.Then([prom, ms, this](){
            std::vector<BlockType> incoming;
            incoming.reserve(uint64_t(ms));
            prom.As(incoming);
            FETCH_LOG_INFO(LOGGING_NAME,"Updating pending blocks: ", incoming.size());
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
    std::vector<BlockType> work;
    if (forward_blocks_.Get(work, 16))
    {
      for (auto &block : work)
      {
        FETCH_LOG_INFO(LOGGING_NAME,"Fowarding block: ", block.hashString());

        Publish(BLOCK_PUBLISH, block);
      }
    }
    if (forward_blocks_.Remaining())
    {
      this->thread_pool_->Post([this]() { this->ForwardBlocks(); });
    }
  }

  void AddPendingBlocks()
  {
    std::vector<BlockType> work;

    if (pending_blocks_.Get(work, 16))
    {
      for (auto &block : work)
      {
        block.UpdateDigest();

        FETCH_LOG_DEBUG(LOGGING_NAME,"OMG Adding? the block to the chain: ", block.summarise());

        if (chain_->AddBlock(block))
        {
          FETCH_LOG_DEBUG(LOGGING_NAME,"OMG Adding the block to the chain: ", block.summarise());

          forward_blocks_.Add(block);
          this->thread_pool_->Post([this]() { this->AddPendingBlocks(); });
          if (block.loose())
          {
            loose_blocks_.Add(block);
            this->thread_pool_->Post([this]() { this->QueryLooseBlocks(); });
          }
        }
      }
    }
    if (pending_blocks_.Remaining())
    {
      this->thread_pool_->Post([this]() { this->AddPendingBlocks(); });
    }
    if (forward_blocks_.Remaining())
    {
      this->thread_pool_->Post([this]() { this->ForwardBlocks(); });
    }
    if (loose_blocks_.Remaining())
    {
      this->thread_pool_->Post([this]() { this->QueryLooseBlocks(); });
    }
  }

  void QueryLooseBlocks()
  {
    std::vector<BlockType> work;
    if (loose_blocks_.Get(work, 16))
    {
      std::vector<block_hash_type> actually_still_loose;
      for (auto &blk : work)
      {
        BlockType tmp;
        if (chain_->Get(blk.hash(), tmp))
        {
          FETCH_LOG_DEBUG(LOGGING_NAME,"OMG LOOOSE?:", tmp.hashString());
          if (tmp.loose())
          {
            actually_still_loose.push_back(blk.hash());
          }
        }
      }
      for(auto &blkhash : actually_still_loose)
      {
        block_type tmp;
        chain_ -> Get(blkhash, tmp);
        FETCH_LOG_DEBUG(LOGGING_NAME,"OMG LOOOSE:", tmp.hashString());

        blockPublishSubscriptions_.VisitSubscriptions(
            [this,blkhash](std::shared_ptr<fetch::service::ServiceClient> client){
              LOG_STACK_TRACE_POINT;

              FETCH_LOG_WARN(LOGGING_NAME,"ERK hash=", ToBase64(blkhash));

              auto prom = client -> Call(protocols::FetchProtocols::MAIN_CHAIN, GET_HEADER, blkhash);
              //auto prom = client -> Call(protocols::FetchProtocols::MAIN_CHAIN, GET_B, ToBase64(blkhash));
              prom.Then([this, prom](){
                  LOG_STACK_TRACE_POINT;
                  std::pair<bool, block_type> result;
                  prom.As(result);
                  if (result.first)
                  {
                    this -> pending_blocks_.Add(result.second);
                    FETCH_LOG_WARN(LOGGING_NAME,"ERK posting catchup block:", ToBase64(result.second.hash()));
                    this -> thread_pool_ -> Post([this]() { this -> AddPendingBlocks(); });
                  }
                  else
                  {
                    FETCH_LOG_ERROR(LOGGING_NAME,"ERK dint have block!");
                  }
                });
              prom.Else([blkhash](){
                  FETCH_LOG_ERROR(LOGGING_NAME,"Something went wrong: ", typeid(blkhash).name() );
                });
            });
      }
    }
    if (loose_blocks_.Remaining())
    {
      this -> thread_pool_ -> Post([this]() { this -> QueryLooseBlocks(); });
    }
  }

#if 0
  void RealisePromises()
  {
    if (!running_) return;
    std::lock_guard<mutex::Mutex> lock(block_list_mutex_);
    incoming_objects_.reserve(uint64_t(max_size_));

    for (auto &p : block_list_promises_)
    {

      if (!running_) return;

      incoming_objects_.clear();
      if (!p.Wait(100, false))
      {
        continue;
      }

      p.template As<std::vector<BlockType>>(incoming_objects_);

      if (!running_) return;
      std::lock_guard<mutex::Mutex> lock(mutex_);

      bool                  loose = false;
      byte_array::ByteArray blockId;

      byte_array::ByteArray prevHash;
      for (auto &block : incoming_objects_)
      {
        block.UpdateDigest();
        chain_->AddBlock(block);
        prevHash = block.prev();
        loose    = block.loose();
      }
      if (loose)
      {
        FETCH_LOG_WARN(LOGGING_NAME,"Loose block");
        TODO("Make list with missing blocks: ", prevHash);
      }
    }

    block_list_promises_.clear();
    if (running_)
    {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); },
                         5000);  // TODO(issue 7): Set from parameter
    }
  }
#endif

  /// @}

  /// RPC
  /// @{
  std::pair<bool, BlockType> GetHeader(block_hash_type const &hash)
  {
    LOG_STACK_TRACE_POINT;
    FETCH_LOG_DEBUG(LOGGING_NAME,"GetHeader starting work");
    BlockType block;
    if (chain_->Get(hash, block))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,"GetHeader done");
      return std::make_pair(true, block);
    }
    else
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,"GetHeader not found");
      return std::make_pair(false, block);
    }
  }

  std::pair<bool, block_type> GetB(const std::string &s)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME,"ERK! GetB ", s);

    std::pair<bool, block_type> r;
    r.first = false;
    return r;
  }

  std::vector<BlockType> GetHeaviestChain(uint32_t const &maxsize)
  {
    LOG_STACK_TRACE_POINT;
    std::vector<BlockType> results;

    FETCH_LOG_DEBUG(LOGGING_NAME,"GetHeaviestChain starting work ", maxsize);

    results = chain_->HeaviestChain(maxsize);

    FETCH_LOG_DEBUG(LOGGING_NAME,"GetHeaviestChain returning ", results.size(), " of req ", maxsize);

    return results;
  }
  /// @}

  chain::MainChain *chain_;
  mutex::Mutex      mutex_{__LINE__, __FILE__};

  generics::WorkItemsQueue<BlockType> pending_blocks_;
  generics::WorkItemsQueue<BlockType> loose_blocks_;
  generics::WorkItemsQueue<BlockType> forward_blocks_;

  std::atomic<bool>     running_;
  std::atomic<uint32_t> max_size_;
  std::string           identifier_;
};

}  // namespace chain
}  // namespace fetch
