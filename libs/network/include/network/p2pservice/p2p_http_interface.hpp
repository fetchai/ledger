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

#include "core/assert.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "network/p2pservice/p2p_service2.hpp"

#include <random>
#include <sstream>

namespace fetch {
namespace p2p {

class P2PHttpInterface : public http::HTTPModule
{
public:
  using MainChain   = chain::MainChain;
  using Muddle      = muddle::Muddle;
  using P2PService  = P2PService2;
  using TrustSystem = P2PTrustInterface<Muddle::Address>;

  P2PHttpInterface(MainChain &chain, Muddle &muddle, P2PService &p2p_service, TrustSystem &trust)
    : chain_(chain)
    , muddle_(muddle)
    , p2p_(p2p_service)
    , trust_(trust)
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
  }

private:
  using Variant = script::Variant;

  http::HTTPResponse GetChainStatus(http::ViewParameters const &params,
                                    http::HTTPRequest const &   request)
  {
    Variant response     = Variant::Object();
    response["identity"] = byte_array::ToBase64(muddle_.identity().identifier());
    response["chain"]    = GenerateBlockList();

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetMuddleStatus(http::ViewParameters const &params,
                                     http::HTTPRequest const &   request)
  {
    auto const connections = muddle_.GetConnections();

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

  http::HTTPResponse GetP2PStatus(http::ViewParameters const &params,
                                  http::HTTPRequest const &   request)
  {
    Variant response           = Variant::Object();
    response["identity_cache"] = GenerateIdentityCache();

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse GetTrustStatus(http::ViewParameters const &params,
                                    http::HTTPRequest const &   request)
  {
    auto const best_peers = trust_.GetBestPeers(100);

    Variant response = Variant::Array(best_peers.size());

    // populate the response
    std::size_t index = 0;
    for (auto const &peer : best_peers)
    {
      Variant peer_data     = Variant::Object();
      peer_data["identity"] = byte_array::ToBase64(peer);
      peer_data["trust"]    = trust_.GetTrustRatingOfPeer(peer);
      peer_data["rank"]     = trust_.GetRankOfPeer(peer);

      response[index++] = peer_data;
    }

    return http::CreateJsonResponse(response);
  }

  Variant GenerateBlockList()
  {
    // lookup the blocks from the heaviest chain
    auto blocks = chain_.HeaviestChain(20);

    Variant block_list = Variant::Array(blocks.size());

    // loop through and generate the complete block list
    std::size_t i = 0;
    for (auto &b : blocks)
    {
      // format the block number
      auto block             = script::Variant::Object();
      block["previous_hash"] = byte_array::ToBase64(b.prev());
      block["current_hash"]  = byte_array::ToBase64(b.hash());
      block["proof"]         = byte_array::ToBase64(b.proof());
      block["block_number"]  = b.body().block_number;

      // store the block in the array
      block_list[i++] = block;
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

  MainChain &  chain_;
  Muddle &     muddle_;
  P2PService & p2p_;
  TrustSystem &trust_;
};

}  // namespace p2p
}  // namespace fetch
