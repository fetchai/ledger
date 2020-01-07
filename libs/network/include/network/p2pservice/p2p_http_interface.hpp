#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/assert.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/state_machine_interface.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "json/document.hpp"
#include "ledger/block_packer_interface.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "logging/logging.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"
#include "version/fetch_version.hpp"

#include <random>
#include <sstream>

namespace fetch {
namespace p2p {

class P2PHttpInterface : public http::HTTPModule
{
public:
  using MainChain            = ledger::MainChain;
  using BlockPackerInterface = ledger::BlockPackerInterface;
  using WeakStateMachine     = std::weak_ptr<core::StateMachineInterface>;
  using WeakStateMachines    = std::vector<WeakStateMachine>;

  static constexpr char const *LOGGING_NAME = "P2PHttpInterface";

  P2PHttpInterface(uint32_t log2_num_lanes, MainChain &chain, BlockPackerInterface &packer,
                   WeakStateMachines state_machines)
    : log2_num_lanes_(log2_num_lanes)
    , chain_(chain)
    , packer_(packer)
    , state_machines_(std::move(state_machines))
  {
    Get("/api/status/chain", "Gets the status of the chain.",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetChainStatus(params, request);
        });
    Get("/api/status/backlog", "Provides mem pool status.",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetBacklogStatus(params, request);
        });
    Get("/api/status/states", "Provides the state of the state machine.",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetStateMachineStatus(params, request);
        });
    Get("/api/status", "Provides high level system status.",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetGeneralStatus(params, request);
        });
  }

private:
  using Variant = variant::Variant;

  http::HTTPResponse GetGeneralStatus(http::ViewParameters const & /*params*/,
                                      http::HTTPRequest const & /*request*/)
  {
    // create the system response
    Variant response    = Variant::Object();
    response["version"] = fetch::version::FULL;
    response["valid"]   = fetch::version::VALID;
    response["lanes"]   = 1u << log2_num_lanes_;

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetChainStatus(http::ViewParameters const & /*params*/,
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

    Variant response  = Variant::Object();
    response["chain"] = GenerateBlockList(include_transactions, chain_length);
    response["block"] = "0x" + chain_.GetHeaviestBlockHash().ToHex();

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetBacklogStatus(http::ViewParameters const & /*params*/,
                                      http::HTTPRequest const & /*request*/)
  {
    variant::Variant data = variant::Variant::Object();
    data["backlog"]       = packer_.GetBacklog();

    return http::CreateJsonResponse(data);
  }

  http::HTTPResponse GetStateMachineStatus(http::ViewParameters const & /*params*/,
                                           http::HTTPRequest const & /*request*/)
  {
    variant::Variant data = variant::Variant::Object();

    for (auto const &sm : state_machines_)
    {
      auto instance = sm.lock();
      if (instance)
      {
        data[instance->GetName()] = instance->GetStateName();
      }
    }

    return http::CreateJsonResponse(data);
  }

  Variant GenerateBlockList(bool include_transactions, std::size_t length)
  {
    // look up the blocks from the heaviest chain
    auto blocks = chain_.GetHeaviestChain(length);

    Variant block_list = Variant::Array(blocks.size());

    // loop through and generate the complete block list
    std::size_t block_idx{0};
    for (auto &b : blocks)
    {
      Variant &block = block_list[block_idx++];

      // format the block number
      block                 = Variant::Object();
      block["hash"]         = "0x" + b->hash.ToHex();
      block["previousHash"] = "0x" + b->previous_hash.ToHex();
      block["merkleHash"]   = "0x" + b->merkle_hash.ToHex();
      block["miner"]        = chain::Address(b->miner_id).display();
      block["blockNumber"]  = b->block_number;
      block["timestamp"]    = b->timestamp;
      block["entropy"]      = b->block_entropy.EntropyAsU64();
      block["weight"]       = b->weight;

      if (include_transactions)
      {
        // create and allocate the variant array
        block["txs"] = Variant::Array(b->GetTransactionCount());

        Variant &tx_list = block["txs"];

        std::size_t tx_idx{0};
        std::size_t slice_idx{0};
        for (auto const &slice : b->slices)
        {
          for (auto const &transaction : slice)
          {
            auto &tx = tx_list[tx_idx];

            tx          = Variant::Object();
            tx["hash"]  = "0x" + transaction.digest().ToHex();
            tx["slice"] = slice_idx;

            ++tx_idx;
          }

          ++slice_idx;
        }
      }
    }

    return block_list;
  }

  uint32_t              log2_num_lanes_;
  MainChain &           chain_;
  BlockPackerInterface &packer_;
  WeakStateMachines     state_machines_;
};

}  // namespace p2p
}  // namespace fetch
