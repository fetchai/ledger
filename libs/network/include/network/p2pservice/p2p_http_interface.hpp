#pragma once

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
#include "ledger/chain/main_chain_service.hpp"

#include <random>
#include <sstream>

namespace fetch {
namespace p2p {

class P2PHttpInterface : public http::HTTPModule
{
public:
  P2PHttpInterface(chain::MainChain *chain,
                   chain::MainChainService::mainchain_protocol_type *protocol)
    : chain_(chain)
    , protocol_(protocol)
  {
    // register all the routes
    Get("/sitrep", [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
      return OnGetSitrep(params, request);
    });
  }

private:
  chain::MainChain *chain_;
  chain::MainChainService::mainchain_protocol_type *protocol_;

  http::HTTPResponse OnGetSitrep(http::ViewParameters const &params,
                                 http::HTTPRequest const &   request)
  {
    auto blocks = chain_->HeaviestChain(20);

    auto ret = script::Variant::Object();

    script::Variant blocklist = script::Variant::Array(blocks.size());
    std::size_t     i   = 0;
    for (auto &b : blocks)
    {
      script::Variant block  = script::Variant::Object();
      block["previous_hash"] = byte_array::ToBase64(b.prev());
      block["hash"]          = byte_array::ToBase64(b.hash());
      block["proof"]         = byte_array::ToBase64(b.proof());
      block["block_number"]  = b.body().block_number;
      block["miner_number"]  = b.body().miner_number;
      blocklist[i]           = block;
      ++i;
    }

    auto chain = script::Variant::Object();
    chain["blocks"] = blocklist;
    ret["chain"] = chain;

    ret["ident"] = protocol_ -> GetIdentity();

    auto subs = protocol_ -> GetCurrentSubscriptions();
    script::Variant subscriptions = script::Variant::Array(subs.size());
    i = 0;
    for (auto &s : subs)
    {
      auto sub = script::Variant::Object();
      sub["weight"] = 1.0;
      sub["peer"] = s;
      subscriptions[i++] = sub;
    }

    ret["subscriptions"] = subscriptions;

    return http::CreateJsonResponse(ret);
  }
};

}  // namespace p2p
}  // namespace fetch
