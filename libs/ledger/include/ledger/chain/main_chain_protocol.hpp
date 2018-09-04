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

#include "network/service/protocol.hpp"
#include <utility>
#include <vector>

namespace fetch {
namespace chain {

template <typename R>
class MainChainProtocol : public fetch::service::Protocol
{
public:
  using BlockType             = chain::MainChain::BlockType;
  using block_hash_type       = chain::MainChain::BlockHash;
  using protocol_handler_type = service::protocol_handler_type;
  using thread_pool_type      = network::ThreadPool;
  using register_type         = R;
  using self_type             = MainChainProtocol<R>;

  enum
  {
    GET_HEADER         = 1,
    GET_HEAVIEST_CHAIN = 2,
  };

  MainChainProtocol(protocol_handler_type const &p, register_type r, thread_pool_type nm,
                    chain::MainChain *node)
    : Protocol()
    , protocol_(p)
    , register_(std::move(r))
    , thread_pool_(std::move(nm))
    , chain_(node)
    , running_(false)
  {
    this->Expose(GET_HEADER, this, &self_type::GetHeader);
    this->Expose(GET_HEAVIEST_CHAIN, this, &self_type::GetHeaviestChain);
    max_size_ = 100;
  }

  void Start()
  {
    fetch::logger.Debug("Starting syncronisation of blocks");
    if (running_)
      return;
    running_ = true;
    thread_pool_->Post([this]() { this->IdleUntilPeers(); });
  }

  void Stop()
  {
    running_ = false;
  }

private:
  protocol_handler_type protocol_;
  register_type         register_;
  thread_pool_type      thread_pool_;

  /// Protocol logic
  /// @{

  void IdleUntilPeers()
  {
    if (!running_)
      return;

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
    fetch::logger.Debug("Fetching blocks from peer");

    if (!running_)
      return;

    std::lock_guard<mutex::Mutex> lock(block_list_mutex_);
    uint32_t                      ms = max_size_;
    using service_map_type           = typename R::service_map_type;
    register_.WithServices([this, ms](service_map_type const &map) {
      for (auto const &p : map)
      {
        if (!running_)
          return;

        auto peer = p.second;
        auto ptr  = peer.lock();
        block_list_promises_.push_back(ptr->Call(protocol_, GET_HEAVIEST_CHAIN, ms));
      }
    });

    if (running_)
    {
      thread_pool_->Post([this]() { this->RealisePromises(); });
    }
  }

  void RealisePromises()
  {
    if (!running_)
      return;
    std::lock_guard<mutex::Mutex> lock(block_list_mutex_);
    incoming_objects_.reserve(uint64_t(max_size_));

    for (auto &p : block_list_promises_)
    {

      if (!running_)
        return;

      incoming_objects_.clear();
      if (!p.Wait(100, false))
      {
        continue;
      }

      p.template As<std::vector<BlockType>>(incoming_objects_);

      if (!running_)
        return;
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
        fetch::logger.Warn("Loose block");
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
  /// @}

  /// RPC
  /// @{
  std::pair<bool, BlockType> GetHeader(block_hash_type const &hash)
  {
    fetch::logger.Debug("GetHeader starting work");
    BlockType block;
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

  std::vector<BlockType> GetHeaviestChain(uint32_t const &maxsize)
  {
    std::vector<BlockType> results;
    std::cerr << "this happened\n\n" << std::endl;

    fetch::logger.Debug("GetHeaviestChain starting work ", maxsize);

    results = chain_->HeaviestChain(maxsize);

    fetch::logger.Debug("GetHeaviestChain returning ", results.size(), " of req ", maxsize);

    return results;
  }
  /// @}

  chain::MainChain *chain_;
  mutex::Mutex      mutex_;

  mutable mutex::Mutex          block_list_mutex_;
  std::vector<service::Promise> block_list_promises_;
  std::vector<BlockType>        incoming_objects_;

  std::atomic<bool>     running_;
  std::atomic<uint32_t> max_size_;
};

}  // namespace chain
}  // namespace fetch
