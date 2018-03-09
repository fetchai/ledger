#ifndef FETCH_NODE_SERVICE_OEF_HPP
#define FETCH_NODE_SERVICE_OEF_HPP

#include"oef/http_oef.hpp"
#include"oef/oef.hpp"
#include"oef/aea_to_node_protocol.hpp"
#include"service/server.hpp"

namespace fetch
{
namespace fetch_node_service
{

class FetchNodeService : public service::ServiceServer< fetch::network::TCPServer >, public http::HTTPServer  {
public:
  FetchNodeService(uint16_t port, fetch::network::ThreadManager *tm) :
    ServiceServer(port, tm),
    HTTPServer(8080, tm) {

    std::shared_ptr<oef::NodeOEF> node      = std::make_shared<oef::NodeOEF>(this);                            // Core OEF functionality - all protocols can access this
    httpOEF_                                = std::make_shared<http_oef::HttpOEF>(node);                       // HTTP interface to node
    aeaToNodeProtocol_                      = std::make_shared<aea_to_node_protocol::AEAToNodeProtocol>(node); // RPC interface to node

    // Add RPC interface AEA->Node
    this->Add(AEAToNodeProtocolID::DEFAULT, aeaToNodeProtocol_.get());

    // HTTP requres this
    this->AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    this->AddMiddleware(fetch::http::middleware::ColorLog);
    this->AddModule(*httpOEF_);
  }

  std::shared_ptr<aea_to_node_protocol::AEAToNodeProtocol> aeaToNodeProtocol_;
  std::shared_ptr<http_oef::HttpOEF>                       httpOEF_;
};
}
}

#endif
