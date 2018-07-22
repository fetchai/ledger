#ifndef EXPLORE_WALLET_HTTP_INTERFACE_HPP
#define EXPLORE_WALLET_HTTP_INTERFACE_HPP

#include "core/json/document.hpp"
#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "http/module.hpp"
#include "http/json_response.hpp"
#include "network/p2pservice/p2p_service.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"

#include <sstream>
#include <random>

namespace fetch {
namespace p2p {

class ExploreHttpInterface : public http::HTTPModule {
public:
  ExploreHttpInterface(p2p::P2PService *p2p /* , ledger::StorageUnitClient *storage, chain::MainChain *chain */)
  : p2p_(p2p) //,
//    storage_(storage),
//    chain_(chain)
  {
    // register all the routes
    Get("/node-entry-points", [this](http::ViewParameters const &params,
                                      http::HTTPRequest const &request) {
      return OnGetEntryPoints(params, request);
    });

    Get("/peer-connections/", [this](http::ViewParameters const &params,
                                      http::HTTPRequest const &request) {
      return OnPeerConnections(params, request);
    });

  }

private:
  p2p::P2PService *p2p_;
//  ledger::StorageUnitClient *storage_;
//  chain::MainChain *chain_;

  http::HTTPResponse OnGetEntryPoints(http::ViewParameters const &params,
                                      http::HTTPRequest const &request) {
    auto profile = p2p_->Profile();

    // TODO(Troels): Note to self - nested accessors does not work: ret["identity"]["xx"] = 2
    script::Variant ident =  script::Variant::Object();
    ident["identifier"] = byte_array::ToBase64(profile.identity.identifier());
    ident["parameters"] = profile.identity.parameters();    

    script::Variant ret = script::Variant::Object();
    ret["identity"] = ident;
    // TODO: This does not work: ret["identity"]["test"] = "blah";

    ret["entry_points"] = script::Variant::Array( profile.entry_points.size() );
    for(std::size_t i=0; i < profile.entry_points.size(); ++i) {
      auto ep = profile.entry_points[i];
      script::Variant jep = script::Variant::Object();

      script::Variant id =  script::Variant::Object();
      id["identifier"] = byte_array::ToBase64(ep.identity.identifier());
      id["parameters"] = ep.identity.parameters();    

      jep["port"] = ep.port;
      jep["lane_id"] = uint32_t(ep.lane_id); 
      jep["is_lane"] = bool(ep.is_lane);
      jep["is_discovery"] = bool(ep.is_discovery);      
      jep["is_mainchain"] = bool(ep.is_mainchain);
      jep["identity"] = id;
      ret["entry_points"][i] = jep;  
    }


    return http::CreateJsonResponse(ret);
  }

  http::HTTPResponse OnPeerConnections(http::ViewParameters const &params,
                                       http::HTTPRequest const &request) {

    return http::CreateJsonResponse(script::Variant());
  }

};

} // namespace p2p
} // namespace fetch

#endif //FETCH_WALLET_HTTP_INTERFACE_HPP
