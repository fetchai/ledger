#ifndef SWARM_HTTP_INTERFACE__
#define SWARM_HTTP_INTERFACE__

#include "http/server.hpp"
#include <iostream>
#include "network/swarm/swarm_node.hpp"


namespace fetch
{
namespace swarm
{

class SwarmHttpInterface : public fetch::http::HTTPModule
{
public:

  explicit SwarmHttpInterface(std::shared_ptr<SwarmNode> node) : node_{node}
  {
    AttachPages();
  }

  SwarmHttpInterface(SwarmHttpInterface&& rhs)
  {
    LOG_STACK_TRACE_POINT ;
    node_ = std::move(rhs.node_);
    AttachPages();
  }

  virtual ~SwarmHttpInterface()
  {
  }

  void AttachPages()
  {
    LOG_STACK_TRACE_POINT ;

    HTTPModule::Get("/peers",
                    [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) \
                    { return this->GetPeers(params, req); }
                    );
  }

  fetch::http::HTTPResponse GetPeers(fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req)
  {
    LOG_STACK_TRACE_POINT ;
    fetch::json::JSONDocument doc;
    try
      {
        fetch::script::Variant result = fetch::script::Variant::Object();

        auto allpeers = node_ -> HttpWantsPeerList();
        auto outputlist = fetch::script::Variant::Array(allpeers.size());

        size_t i=0;
        for(auto &p: allpeers )
          {
            auto peerObject = fetch::script::Variant::Object();
            peerObject["peer"] = p.GetLocation().AsString();
            peerObject["weight"] = p.GetKarma();
            outputlist[i++]=peerObject;
          }

        result["peers"] = outputlist;
        result["state"] = node_ -> GetState();

        std::ostringstream ret;
        ret << result;
        return fetch::http::HTTPResponse(ret.str());
      } catch (...)
      {
        return fetch::http::HTTPResponse(failureString);
      }
  }

  SwarmHttpInterface(SwarmHttpInterface &rhs)            = delete;
  SwarmHttpInterface operator=(SwarmHttpInterface &rhs)  = delete;
  SwarmHttpInterface operator=(SwarmHttpInterface &&rhs) = delete;

private:
  std::shared_ptr<SwarmNode> node_;
  const std::string successString{"{\"response\": \"success\" }"};
  const std::string failureString{"{\"response\": \"failure\", \"reason\": \"problems with parsing JSON!\"}"};};
}
}

#endif //__SWARM_HTTP_INTERFACE__
