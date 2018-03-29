#ifndef FETCH_NODE_SERVICE_OEF_HPP
#define FETCH_NODE_SERVICE_OEF_HPP

#include"oef/http_oef.hpp"
#include"oef/oef.hpp"
#include"service/server.hpp"
#include"protocols/aea_to_node.hpp"
#include"protocols/node_to_node.hpp"

// Fetch node service connects the desired protocols to the OEF API

namespace fetch
{
namespace fetch_node_service
{

class FetchNodeService : public service::ServiceServer< fetch::network::TCPServer >, public http::HTTPServer {
public:
  //FetchNodeService(fetch::network::ThreadManager *tm, uint16_t tcpPort, uint16_t httpPort, std::string configFile) :
  FetchNodeService(fetch::network::ThreadManager *tm, uint16_t tcpPort, uint16_t httpPort, const schema::Instance &instance, const schema::Endpoint &nodeEndpoint, const schema::Endpoints &endpoints) :
    ServiceServer(tcpPort, tm),
    HTTPServer(httpPort, tm) {
    fetch::logger.Debug("Constructing fetch node service with TCP port: ", tcpPort, " and HTTP port: ", httpPort);

    node_               = std::make_shared<oef::NodeOEF>(this, tm, instance, nodeEndpoint, endpoints);  // Core OEF functionality - all protocols can access this
    httpOEF_            = std::make_shared<http_oef::HttpOEF>(node_);                  // HTTP interface to node
    aeaToNodeProtocol_  = std::make_shared<protocols::AEAToNodeProtocol>(node_);       // RPC AEA interface to node
    nodeToNodeProtocol_ = std::make_shared<protocols::NodeToNodeProtocol>(node_);      // RPC Node interface to node

    // Add RPC interface AEA->Node. Note this allows the Node to callback to AEAs too
    this->Add(protocols::FetchProtocols::AEA_TO_NODE, aeaToNodeProtocol_.get());
    this->Add(protocols::FetchProtocols::NODE_TO_NODE, nodeToNodeProtocol_.get());

    // Add middleware to the HTTP server - allow requests from any address, and print requests to the terminal in colour
    this->AddMiddleware(http::middleware::AllowOrigin("*"));
    this->AddMiddleware(http::middleware::ColorLog);
    this->AddModule(*httpOEF_);
  }

  void Start() {
    node_->Start();
  }

private:
  std::shared_ptr<oef::NodeOEF>                  node_;
  std::shared_ptr<protocols::AEAToNodeProtocol>  aeaToNodeProtocol_;
  std::shared_ptr<protocols::NodeToNodeProtocol> nodeToNodeProtocol_;
  std::shared_ptr<http_oef::HttpOEF>             httpOEF_;
};
}
}

#endif
