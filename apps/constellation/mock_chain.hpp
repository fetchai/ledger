#pragma once
#include "ledger/chain/block.hpp"
#include "core/mutex.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "miner/resource_mapper.hpp"

#include <vector>
#include <functional>
#include <algorithm>
#include <random>
#include <sstream>

namespace fetch
{
namespace ledger
{

class MockChain
{
public:
  using BlockEventCallback = std::function< void(Block) >;
  using Digest = byte_array::ConstByteArray;
  using DigestArray = std::vector< Digest >;
  using ThreadPool      = network::ThreadPool;
  using DigestSet         = std::unordered_set<Digest>;

  MockChain()
  {
    Block genesis{};
    genesis.body.previous_hash = GENESIS_DIGEST;
    genesis.is_loose           = false;
    genesis.UpdateDigest();

    chain_.push_back(genesis);

    thread_pool_ = network::MakeThreadPool(1, "MockChain Thread Pool");    
  }

  void Start()
  {
    thread_pool_->Start();
    thread_pool_->Post([this]() { BlockCycle(); });
  }


  void Stop()
  {
    thread_pool_->Stop();
  }

  void BlockCycle()
  {
    FETCH_LOCK(chain_mutex_);
    MakeBlock();
    thread_pool_->Post([this]() { BlockCycle(); }, 25000);
  }

  void OnBlock(BlockEventCallback on_block )
  {
    FETCH_LOCK(chain_mutex_);    
    on_block_ = on_block;
  }

  std::vector< Block > HeaviestChain(int const &n)
  {
    FETCH_LOCK(chain_mutex_);    
    std::vector< Block > blocks;
    int q = int(chain_.size()) - n;

    if(q < 0) 
    {
      q = 0;
    }

    for(; q < int(chain_.size()); ++q)
    {
      blocks.push_back( chain_[static_cast<uint64_t>(q)] );
    }
    return blocks;
  }

  void SetTips(DigestSet tips) 
  {
    FETCH_LOCK(chain_mutex_);    
    tips_ = tips;
  }
private:
  void MakeBlock()
  {
    Block       next_block;
    Block::Body next_block_body;
    Block const &last = chain_.back();

    next_block_body.previous_hash = last.body.hash;
    next_block_body.block_number = last.body.block_number + 1;

    std::copy(tips_.begin(), tips_.end(), std::back_inserter(next_block_body.dag_nodes));
    tips_.clear();

    next_block.body =     next_block_body;
    next_block.UpdateDigest();

    chain_.push_back(next_block);
    if(on_block_) on_block_(next_block);
  }



  BlockEventCallback on_block_{nullptr};
  ThreadPool thread_pool_;
  fetch::mutex::Mutex chain_mutex_{__LINE__, __FILE__};
  std::vector< Block > chain_;
  DigestSet tips_;  
};


class MockChainHTTPInterface : public http::HTTPModule
{
public:
  using MainChain   = ledger::MainChain;

  static constexpr char const *LOGGING_NAME = "MockChainHTTPInterface";

  MockChainHTTPInterface(uint32_t log2_num_lanes, MockChain &chain)
    : log2_num_lanes_(log2_num_lanes)
    , chain_(chain)
  {
    Get("/api/mock-chain/list-blocks",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetChain(params, request);
        });

  }

private:
  using Variant = variant::Variant;

  http::HTTPResponse GetChain(http::ViewParameters const & /*params*/,
                                    http::HTTPRequest const &request)
  {
    std::size_t chain_length         = 20;
    bool        include_transactions = false;

    if (request.query().Has("size"))
    {
      chain_length = static_cast<std::size_t>(request.query()["size"].AsInt());
    }

    if (request.query().Has("tx"))
    {
      include_transactions = true;
    }

    Variant response     =  GenerateBlockList(include_transactions, chain_length);

    return http::CreateJsonResponse(response);
  }

  Variant GenerateBlockList(bool include_transactions, std::size_t length)
  {
    using byte_array::ToBase64;

    // lookup the blocks from the heaviest chain
    auto blocks = chain_.HeaviestChain(int(length));

    Variant block_list = Variant::Array(blocks.size());

    // loop through and generate the complete block list
    std::size_t block_idx{0};
    for (auto &b : blocks)
    {
      // format the block number
      auto block = Variant::Object();
      auto dag_nodes = Variant::Array(b.body.dag_nodes.size());

      int dag_idx = 0;
      for(auto const&n: b.body.dag_nodes)
      {
        dag_nodes[static_cast<uint64_t>(dag_idx++)] = byte_array::ToBase64(n);
      }


      block["hash"]         = byte_array::ToBase64(b.body.hash);
      block["previousHash"] = byte_array::ToBase64(b.body.previous_hash);
      block["merkleHash"]   = byte_array::ToBase64(b.body.merkle_hash);
      block["proof"]        = byte_array::ToBase64(b.proof.digest());
      block["miner"]        = byte_array::ToBase64(b.body.miner);
      block["blockNumber"]  = b.body.block_number;
      block["dag_nodes"] = dag_nodes;

      if (include_transactions)
      {
        auto const &slices = b.body.slices;

        Variant slice_list = Variant::Array(slices.size());

        std::size_t slice_idx{0};
        for (auto const &slice : slices)
        {
          Variant transaction_list = Variant::Array(slice.size());

          std::size_t tx_idx{0};
          for (auto const &transaction : slice)
          {
            Variant tx_obj         = Variant::Object();
            tx_obj["digest"]       = ToBase64(transaction.transaction_hash);
            tx_obj["fee"]          = transaction.fee;
            tx_obj["contractName"] = transaction.contract_name;

            Variant resources_array = Variant::Array(transaction.resources.size());

            std::size_t res_idx{0};
            for (auto const &resource : transaction.resources)
            {
              Variant res_obj     = Variant::Object();
              res_obj["resource"] = ToBase64(resource);
              res_obj["lane"] =
                  miner::MapResourceToLane(resource, transaction.contract_name, log2_num_lanes_);

              resources_array[res_idx++] = res_obj;
            }

            tx_obj["resources"]        = resources_array;
            transaction_list[tx_idx++] = tx_obj;
          }

          slice_list[slice_idx++] = transaction_list;
        }

        block["slices"] = slice_list;
      }

      // store the block in the array
      block_list[block_idx++] = block;
    }

    return block_list;
  }

  uint32_t     log2_num_lanes_;
  MockChain &  chain_;
};





}
}