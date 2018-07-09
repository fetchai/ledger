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
                        fetch::network::NetworkManager tm,
                        uint16_t httpPort,
                        std::shared_ptr<SwarmNode> node,
                        const std::string &hostname,
                        unsigned int idlespeed
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
    httpInterface_ = std::make_shared<SwarmHttpInterface>(node_);
    rpcInterface_ = std::make_shared<SwarmProtocol>(node_);

    // Add middleware to the HTTP server - allow requests from any address,
    // and print requests to the terminal in colour
    this->AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    this->AddMiddleware(fetch::http::middleware::ColorLog);
    this->AddModule(*httpInterface_);

    addRpcProtocol(protocols::FetchProtocols::SWARM, rpcInterface_.get());
  }

  void addRpcProtocol(unsigned int protocolNumber, fetch::service::Protocol *proto)
  {
    this -> Add(protocolNumber, proto);
  }

  template<class PROTOCOL_DESCENDENT>
  void addRpcProtocol(unsigned int protocolNumber, std::shared_ptr<PROTOCOL_DESCENDENT> sptr)
  {
    this -> addRpcProtocol(protocolNumber, sptr.get());
  }

  virtual ~SwarmService()
  {
  }

protected:
  uint16_t                            port_;
  std::shared_ptr<SwarmNode>          node_;
  std::shared_ptr<SwarmHttpInterface> httpInterface_;
  std::shared_ptr<SwarmProtocol>      rpcInterface_;
  fetch::network::NetworkManager       tm_;
};

}
}

#endif //__SWARM_SERVICE__
