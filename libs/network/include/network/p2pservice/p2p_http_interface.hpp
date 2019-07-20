#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "core/state_machine_interface.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "ledger/block_packer_interface.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "miner/resource_mapper.hpp"
#include "network/p2pservice/p2p_service.hpp"
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
  using Muddle               = muddle::Muddle;
  using TrustSystem          = P2PTrustInterface<Muddle::Address>;
  using BlockPackerInterface = ledger::BlockPackerInterface;
  using WeakStateMachine     = std::weak_ptr<core::StateMachineInterface>;
  using WeakStateMachines    = std::vector<WeakStateMachine>;

  static constexpr char const *LOGGING_NAME = "P2PHttpInterface";

  P2PHttpInterface(uint32_t log2_num_lanes, MainChain &chain, Muddle &muddle,
                   P2PService &p2p_service, TrustSystem &trust, BlockPackerInterface &packer,
                   WeakStateMachines state_machines)
    : log2_num_lanes_(log2_num_lanes)
    , chain_(chain)
    , muddle_(muddle)
    , p2p_(p2p_service)
    , trust_(trust)
    , packer_(packer)
    , state_machines_(std::move(state_machines))
  {
    Get("/api/status/chain",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetChainStatus(params, request);
        });
    Get("/api/status/muddle",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetMuddleStatus(params, request);
        });
    Get("/api/status/p2p",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetP2PStatus(params, request);
        });
    Get("/api/status/trust",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetTrustStatus(params, request);
        });
    Get("/api/status/backlog",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetBacklogStatus(params, request);
        });
    Get("/api/status/states",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetStateMachineStatus(params, request);
        });
    Get("/api/status",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return GetGeneralStatus(params, request);
        });
  }

private:
  using Variant = variant::Variant;

  http::HTTPResponse GetGeneralStatus(http::ViewParameters const &, http::HTTPRequest const &)
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

    Variant response     = Variant::Object();
    response["chain"]    = GenerateBlockList(include_transactions, chain_length);
    response["identity"] = fetch::byte_array::ToBase64(muddle_.identity().identifier());
    response["block"]    = fetch::byte_array::ToBase64(chain_.GetHeaviestBlockHash());

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetMuddleStatus(http::ViewParameters const & /*params*/,
                                     http::HTTPRequest const & /*request*/)
  {
    auto const connections = muddle_.GetConnections(true);

    Variant response = Variant::Array(connections.size());

    std::size_t index = 0;
    for (auto const &entry : connections)
    {
      Variant object = Variant::Object();

      object["identity"] = byte_array::ToBase64(entry.first);
      object["uri"]      = entry.second.uri();

      response[index++] = object;
    }

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetP2PStatus(http::ViewParameters const & /*params*/,
                                  http::HTTPRequest const & /*request*/)
  {
    Variant response          = Variant::Object();
    response["identityCache"] = GenerateIdentityCache();

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetTrustStatus(http::ViewParameters const & /*params*/,
                                    http::HTTPRequest const & /*request*/)
  {
    auto peers_trusts = trust_.GetPeersAndTrusts();

    std::vector<variant::Variant> peer_data_list;

    for (auto const &pt : peers_trusts)
    {
      variant::Variant peer_data = variant::Variant::Object();
      peer_data["target"]        = pt.name;
      peer_data["blacklisted"]   = muddle_.IsBlacklisted(pt.address);
      peer_data["value"]         = pt.trust;
      peer_data["active"]        = muddle_.IsConnected(pt.address);
      peer_data["desired"]       = p2p_.IsDesired(pt.address);
      peer_data["source"]        = byte_array::ToBase64(muddle_.identity().identifier());

      peer_data_list.push_back(peer_data);
    }

    variant::Variant trust_list = variant::Variant::Array(peer_data_list.size());
    for (std::size_t i = 0; i < peer_data_list.size(); i++)
    {
      trust_list[i] = peer_data_list[i];
    }

    Variant response     = Variant::Object();
    response["identity"] = fetch::byte_array::ToBase64(muddle_.identity().identifier());
    response["trusts"]   = trust_list;

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
    // lookup the blocks from the heaviest chain
    auto blocks = chain_.GetHeaviestChain(length);

    Variant block_list = Variant::Array(blocks.size());

    // loop through and generate the complete block list
    std::size_t block_idx{0};
    for (auto &b : blocks)
    {
      Variant &block = block_list[block_idx++];

      // format the block number
      block                 = Variant::Object();
      block["hash"]         = "0x" + b->body.hash.ToHex();
      block["previousHash"] = "0x" + b->body.previous_hash.ToHex();
      block["merkleHash"]   = "0x" + b->body.merkle_hash.ToHex();
      block["proof"]        = "0x" + b->proof.header().ToHex();
      block["miner"]        = b->body.miner.display();
      block["blockNumber"]  = b->body.block_number;
      block["timestamp"]    = b->body.timestamp;

      if (include_transactions)
      {
        // create and allocate the variant array
        block["txs"] = Variant::Array(b->GetTransactionCount());

        Variant &tx_list = block["txs"];

        std::size_t tx_idx{0};
        std::size_t slice_idx{0};
        for (auto const &slice : b->body.slices)
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

  Variant GenerateIdentityCache()
  {
    // make a copy of the identity cache
    IdentityCache::Cache cache_copy;
    p2p_.identity_cache().VisitCache(
        [&cache_copy](IdentityCache::Cache const &cache) { cache_copy = cache; });

    auto cache = Variant::Array(cache_copy.size());

    std::size_t index = 0;
    for (auto const &entry : cache_copy)
    {
      // format the individual entries
      auto cache_entry        = Variant::Object();
      cache_entry["identity"] = byte_array::ToBase64(entry.first);
      cache_entry["uri"]      = entry.second.uri.uri();

      // add it to the main cache representation
      cache[index++] = cache_entry;
    }

    return cache;
  }

  uint32_t              log2_num_lanes_;
  MainChain &           chain_;
  Muddle &              muddle_;
  P2PService &          p2p_;
  TrustSystem &         trust_;
  BlockPackerInterface &packer_;
  WeakStateMachines     state_machines_;
};

}  // namespace p2p
}  // namespace fetch
