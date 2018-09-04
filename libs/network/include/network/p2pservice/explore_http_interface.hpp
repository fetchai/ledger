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
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "network/p2pservice/p2p_service.hpp"

#include <random>
#include <sstream>

namespace fetch {
namespace p2p {

class ExploreHttpInterface : public http::HTTPModule
{
public:
  ExploreHttpInterface(p2p::P2PService *p2p, chain::MainChain *chain)
    : p2p_(p2p)
    , chain_(chain)
  {
    // register all the routes
    Get("/node-entry-points",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return OnGetEntryPoints(params, request);
        });

    Get("/peer-connections/",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return OnPeerConnections(params, request);
        });

    Get("/get-chain", [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
      return OnGetChain(params, request);
    });
  }

private:
  p2p::P2PService * p2p_;
  chain::MainChain *chain_;

  http::HTTPResponse OnGetEntryPoints(http::ViewParameters const &params,
                                      http::HTTPRequest const &   request)
  {
    auto profile = p2p_->Profile();

    // TODO(issue 27): Note to self - nested accessors does not work:
    // ret["identity"]["xx"] = 2
    script::Variant ident = script::Variant::Object();
    ident["identifier"]   = byte_array::ToBase64(profile.identity.identifier());
    ident["parameters"]   = profile.identity.parameters();

    script::Variant ret = script::Variant::Object();
    ret["identity"]     = ident;
    ret["entry_points"] = script::Variant::Array(profile.entry_points.size());

    for (std::size_t i = 0; i < profile.entry_points.size(); ++i)
    {
      auto            ep  = profile.entry_points[i];
      script::Variant jep = script::Variant::Object();

      script::Variant id = script::Variant::Object();
      id["identifier"]   = byte_array::ToBase64(ep.identity.identifier());
      id["parameters"]   = ep.identity.parameters();

      script::Variant host = script::Variant::Array(ep.host.size());
      std::size_t     j    = 0;
      for (auto &h : ep.host)
      {
        host[j++] = h;
      }

      jep["host"]            = host;
      jep["port"]            = ep.port;
      jep["lane_id"]         = uint32_t(ep.lane_id);
      jep["is_lane"]         = bool(ep.is_lane);
      jep["is_discovery"]    = bool(ep.is_discovery);
      jep["is_mainchain"]    = bool(ep.is_mainchain);
      jep["identity"]        = id;
      ret["entry_points"][i] = jep;
    }

    return http::CreateJsonResponse(ret);
  }

  http::HTTPResponse OnGetChain(http::ViewParameters const &params,
                                http::HTTPRequest const &   request)
  {
    auto blocks = chain_->HeaviestChain(20);

    script::Variant ret = script::Variant::Array(blocks.size());
    std::size_t     i   = 0;
    for (auto &b : blocks)
    {
      script::Variant block  = script::Variant::Object();
      block["previous_hash"] = byte_array::ToBase64(b.prev());
      block["hash"]          = byte_array::ToBase64(b.hash());
      block["proof"]         = byte_array::ToBase64(b.proof());
      block["block_number"]  = b.body().block_number;
      block["miner_number"]  = b.body().miner_number;
      ret[i]                 = block;
      ++i;
    }

    return http::CreateJsonResponse(ret);
  }

  http::HTTPResponse OnPeerConnections(http::ViewParameters const &params,
                                       http::HTTPRequest const &   request)
  {

    return http::CreateJsonResponse(script::Variant());
  }
};

}  // namespace p2p
}  // namespace fetch
