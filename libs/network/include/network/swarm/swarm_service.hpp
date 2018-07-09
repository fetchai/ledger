#ifndef SWARM_SERVICE__
#define SWARM_SERVICE__

#include "http/server.hpp"
#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"

#include "network/protocols/swarm/swarm_protocol.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "swarm_agent_api.hpp"
#include "swarm_http_interface.hpp"

#include <iostream>


namespace fetch
{
namespace swarm
{

  using std::cout;
  using std::cerr;
  using std::cerr;

class SwarmService :
    public fetch::http::HTTPServer,
    public service::ServiceServer<fetch::network::TCPServer>
{
public:
  SwarmService(SwarmService &rhs)            = delete;
  SwarmService operator=(SwarmService &rhs)  = delete;
  SwarmService operator=(SwarmService &&rhs) = delete;

  explicit SwarmService(
                        fetch::network::ThreadManager tm,
                        uint16_t httpPort,
                        std::shared_ptr<SwarmNode> node,
                        const std::string &hostname,
                        uint32_t idlespeed
                        ) :
    HTTPServer(uint16_t(httpPort+1000), tm),
    ServiceServer(httpPort, tm),
    tm_(tm)
  {
    LOG_STACK_TRACE_POINT ;
    fetch::logger.Debug("Constructing test node service with ",
                        "HTTP port: ", httpPort);

    port_ = httpPort;
    node_ = node;
    httpModule_ = std::make_shared<SwarmHttpModule>(node);
    rpcInterface_ = std::make_shared<SwarmProtocol>(node_.get());

    // Add middleware to the HTTP server - allow requests from any address,
    // and print requests to the terminal in colour
    this->AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    this->AddMiddleware(fetch::http::middleware::ColorLog);

    this->AddModule(*httpModule_);

    addRpcProtocol(protocols::FetchProtocols::SWARM, rpcInterface_.get());
  }

  void addRpcProtocol(uint32_t protocolNumber, fetch::service::Protocol *proto)
  {
    this -> Add(protocolNumber, proto);
  }

  template<class PROTOCOL_DESCENDENT>
  void addRpcProtocol(uint32_t protocolNumber, std::shared_ptr<PROTOCOL_DESCENDENT> sptr)
  {
    this -> addRpcProtocol(protocolNumber, sptr.get());
  }

  virtual ~SwarmService()
  {
  }

protected:
  uint16_t                            port_;
  std::shared_ptr<SwarmNode>          node_;
  std::shared_ptr<SwarmHttpModule> httpModule_;
  std::shared_ptr<SwarmProtocol>      rpcInterface_;
  fetch::network::ThreadManager       tm_;
};

}
}

#endif //__SWARM_SERVICE__
