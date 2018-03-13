#ifndef FETCH_NODE_SERVICE_OEF_HPP
#define FETCH_NODE_SERVICE_OEF_HPP

#include"oef/http_oef.hpp"
#include"oef/oef.hpp"
#include"service/server.hpp"
#include"protocols/aea_to_node.hpp"

namespace fetch
{
namespace fetch_node_service
{

class FetchNodeService : public service::ServiceServer< fetch::network::TCPServer >, public http::HTTPServer {
public:
  FetchNodeService(uint16_t port, fetch::network::ThreadManager *tm) :
    ServiceServer(port, tm),
    HTTPServer(8080, tm) {

    std::shared_ptr<oef::NodeOEF> node      = std::make_shared<oef::NodeOEF>(this);                            // Core OEF functionality - all protocols can access this
    httpOEF_                                = std::make_shared<http_oef::HttpOEF>(node);                       // HTTP interface to node
    aeaToNodeProtocol_                      = std::make_shared<protocols::AEAToNodeProtocol>(node); // RPC interface to node

    // Add RPC interface AEA->Node. Note this allows the Node to callback to AEAs too
    this->Add(protocols::FetchProtocols::AEA_TO_NODE, aeaToNodeProtocol_.get());

    // Add middleware to the HTTP server - allow requests from any address, and print requests to the terminal in colour
    this->AddMiddleware(http::middleware::AllowOrigin("*"));
    this->AddMiddleware(http::middleware::ColorLog);
    this->AddModule(*httpOEF_);
  }

  std::shared_ptr<protocols::AEAToNodeProtocol> aeaToNodeProtocol_;
  std::shared_ptr<http_oef::HttpOEF>            httpOEF_;
};
}
}

#endif
