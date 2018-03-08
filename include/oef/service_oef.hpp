#ifndef SERVICE_OEF_HPP
#define SERVICE_OEF_HPP

#include"oef/http_oef.hpp"
#include"oef/oef.hpp"
#include"oef/rpc_protocol_aea.hpp"
#include"service/server.hpp"

namespace fetch
{
namespace service_oef
{

class ServiceOEF : public service::ServiceServer< fetch::network::TCPServer >, public http::HTTPServer  {
public:
  ServiceOEF(uint16_t port, fetch::network::ThreadManager *tm) :
    ServiceServer(port, tm),
    HTTPServer(8080, tm) {

    std::shared_ptr<oef::NodeOEF> node = std::make_shared<oef::NodeOEF>();                    // Core OEF functionality - all protocols can access this
    httpOEF_                                = std::make_shared<http_oef::HttpOEF>(node);                // HTTP interface to node
    rpcProtocol_                            = std::make_shared<rpc_protocol_aea::RpcProtocolAEA>(node); // RPC interface to node

    // Add RPC interface
    this->Add(AEAProtocolEnum::DEFAULT, rpcProtocol_.get());

    // HTTP requres this
    this->AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    this->AddMiddleware(fetch::http::middleware::ColorLog);
    this->AddModule(*httpOEF_);
  }

  std::shared_ptr<rpc_protocol_aea::RpcProtocolAEA> rpcProtocol_;
  std::shared_ptr<http_oef::HttpOEF>                httpOEF_;
};
}
}

#endif
