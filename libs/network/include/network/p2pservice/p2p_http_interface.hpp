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
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "miner/resource_mapper.hpp"
#include "network/p2pservice/p2p_service.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"

#include <random>
#include <sstream>

namespace fetch {
namespace p2p {

class P2PHttpInterface : public http::HTTPModule
{
public:
  using MainChain        = chain::MainChain;
  using Muddle           = muddle::Muddle;
  using TrustSystem      = P2PTrustInterface<Muddle::Address>;
  using Miner            = miner::MinerInterface;
  using MainChainService = std::shared_ptr<ledger::MainChainRpcService>;

  static constexpr char const *LOGGING_NAME = "P2PHttpInterface";

  P2PHttpInterface(uint32_t log2_num_lanes, MainChain &chain, Muddle &muddle,
                   P2PService &p2p_service, TrustSystem &trust, Miner &miner,
                   MainChainService main_chain_service)
    : log2_num_lanes_(log2_num_lanes)
    , chain_(chain)
    , muddle_(muddle)
    , p2p_(p2p_service)
    , trust_(trust)
    , miner_(miner)
    , main_chain_service_(std::move(main_chain_service))
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
  }

private:
  using Variant = variant::Variant;

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
    response["block"]    = fetch::byte_array::ToBase64(chain_.HeaviestBlock().hash());

    // TODO(private issue 532): Remove legacy API
    response["i_am"]      = fetch::byte_array::ToBase64(muddle_.identity().identifier());
    response["block_hex"] = fetch::byte_array::ToHex(chain_.HeaviestBlock().hash());
    response["i_am_hex"]  = fetch::byte_array::ToHex(muddle_.identity().identifier());

    Variant history = Variant::Object();
    auto block_history = main_chain_service_->GetBlockHistory();
    for (auto &d : block_history)
    {
      Variant details = Variant::Array(d.second.size());//(details_tmp.size());
      for (std::size_t i = 0; i < d.second.size(); ++i)
      {
        Variant e        = Variant::Object();
        e["from"]        = byte_array::ToBase64(d.second[i].from);
        e["transmitter"] = byte_array::ToBase64(d.second[i].transmitter);
        e["time"]        = d.second[i].time;
        e["type"]        = ledger::MainChainRpcService::ToString(d.second[i].type);
        e["first"]       = d.second[i].first;
        details[i]       = e;
      }
      std::string block_hash = static_cast<std::string>(ToBase64(d.first));
      history[block_hash] = details;
    }
    response["history"] = history;

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetMuddleStatus(http::ViewParameters const & /*params*/,
                                     http::HTTPRequest const & /*request*/)
  {
    auto const connections = muddle_.GetConnections(true);

    std::vector<variant::Variant> connections_output_list;

    for (auto const &entry : connections)
    {
      if (muddle_.IsConnected(entry.first))
      {
        continue;
      }

      Variant object     = Variant::Object();
      object["identity"] = byte_array::ToBase64(entry.first);
      object["uri"]      = entry.second.uri();
      connections_output_list.push_back(object);
    }

    variant::Variant response = variant::Variant::Array(connections_output_list.size());
    for (std::size_t i = 0; i < connections_output_list.size(); i++)
    {
      response[i] = connections_output_list[i];
    }

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetP2PStatus(http::ViewParameters const & /*params*/,
                                  http::HTTPRequest const & /*request*/)
  {
    Variant response          = Variant::Object();
    response["identityCache"] = GenerateIdentityCache();

    // TODO(private issue 532): Remove legacy API
    response["identity_cache"] = GenerateIdentityCache();

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetTrustStatus(http::ViewParameters const & /*params*/,
                                    http::HTTPRequest const & /*request*/) {
    auto peers_trusts = trust_.GetPeersAndTrusts();

    std::vector<variant::Variant> peer_data_list;

    for (const auto &pt : peers_trusts) {
      variant::Variant peer_data = variant::Variant::Object();
      peer_data["target"]       = pt.name;
      peer_data["value"]        = pt.trust;
      peer_data["source"]       = byte_array::ToBase64(muddle_.identity().identifier());
      peer_data["blacklisted"]  = muddle_.IsBlacklisted(pt.address);
      peer_data["active"]       = muddle_.IsConnected(pt.address);
      peer_data["desired"]      = p2p_.IsDesired(pt.address);
      peer_data["experimental"] = p2p_.IsExperimental(pt.address);
      peer_data_list.emplace_back(std::move(peer_data));
    }

    variant::Variant trust_list = variant::Variant::Array(peers_trusts.size());
    for (std::size_t i = 0; i < peer_data_list.size(); i++) {
      trust_list[i] = peer_data_list[i];
    }

    Variant response = Variant::Object();
    response["identity"] = fetch::byte_array::ToBase64(muddle_.identity().identifier());
    response["trusts"] = trust_list;

    // TODO(private issue 532): Remove legacy API
    response["i_am"]      = fetch::byte_array::ToBase64(muddle_.identity().identifier());
    response["block"]     = fetch::byte_array::ToBase64(chain_.HeaviestBlock().hash());
    response["block_hex"] = fetch::byte_array::ToHex(chain_.HeaviestBlock().hash());
    response["i_am_hex"]  = fetch::byte_array::ToHex(muddle_.identity().identifier());

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetBacklogStatus(http::ViewParameters const & /*params*/,
                                      http::HTTPRequest const & /*request*/)
  {
    variant::Variant data = variant::Variant::Object();
    data["backlog"]       = miner_.GetBacklog();

    return http::CreateJsonResponse(data);
  }

  Variant GenerateBlockList(bool include_transactions, std::size_t length)
  {
    using byte_array::ToBase64;

    // lookup the blocks from the heaviest chain
    auto blocks = chain_.HeaviestChain(length);

    Variant block_list = Variant::Array(blocks.size());

    // loop through and generate the complete block list
    std::size_t block_idx{0};
    for (auto &b : blocks)
    {
      // format the block number
      auto block = Variant::Object();

      block["hash"]         = byte_array::ToBase64(b.hash());
      block["previousHash"] = byte_array::ToBase64(b.prev());
      block["merkleHash"]   = byte_array::ToBase64(b.body().merkle_hash);
      block["proof"]        = byte_array::ToBase64(b.proof().header());
      block["miner"]        = byte_array::ToBase64(b.body().miner);
      block["blockNumber"]  = b.body().block_number;

      // TODO(private issue 532): Remove legacy API
      block["currentHash"] = byte_array::ToBase64(b.hash());

      if (include_transactions)
      {
        auto const &slices = b.body().slices;

        Variant slice_list = Variant::Array(slices.size());

        std::size_t slice_idx{0};
        for (auto const &slice : slices)
        {
          Variant transaction_list = Variant::Array(slice.transactions.size());

          std::size_t tx_idx{0};
          for (auto const &transaction : slice.transactions)
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

  uint32_t    log2_num_lanes_;
  MainChain & chain_;
  Muddle &    muddle_;
  P2PService &p2p_;

  TrustSystem &    trust_;
  Miner &          miner_;
  MainChainService main_chain_service_;
};

}  // namespace p2p
}  // namespace fetch
