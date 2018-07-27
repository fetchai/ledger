#pragma once

#include "http/server.hpp"
#include "network/swarm/swarm_node.hpp"
#include <iostream>

namespace fetch {
namespace swarm {

class SwarmHttpModule : public fetch::http::HTTPModule
{
public:
  explicit SwarmHttpModule(std::shared_ptr<SwarmNode> node) : node_{node} { AttachPages(); }

  SwarmHttpModule(SwarmHttpModule &&rhs)
  {
    LOG_STACK_TRACE_POINT;
    node_ = std::move(rhs.node_);
    AttachPages();
  }

  virtual ~SwarmHttpModule() = default;

  void AttachPages()
  {
    LOG_STACK_TRACE_POINT;

    HTTPModule::Get("/peers", [this](fetch::http::ViewParameters const &params,
                                     fetch::http::HTTPRequest const &   req) {
      return this->GetPeers(params, req);
    });
  }

  fetch::http::HTTPResponse GetPeers(fetch::http::ViewParameters const &params,
                                     fetch::http::HTTPRequest const &   req)
  {
    LOG_STACK_TRACE_POINT;
    fetch::json::JSONDocument doc;
    try
    {
      fetch::script::Variant result = fetch::script::Variant::Object();

      auto allpeers   = node_->HttpWantsPeerList();
      auto outputlist = fetch::script::Variant::Array(allpeers.size());

      size_t i = 0;
      for (auto &p : allpeers)
      {
        auto peerObject      = fetch::script::Variant::Object();
        peerObject["peer"]   = p.GetLocation().AsString();
        peerObject["weight"] = p.GetKarma();
        outputlist[i++]      = peerObject;
      }

      result["peers"] = outputlist;
      result["state"] = node_->GetState();

      std::ostringstream ret;
      ret << result;
      return fetch::http::HTTPResponse(ret.str());
    }
    catch (...)
    {
      return fetch::http::HTTPResponse(failureString);
    }
  }

  SwarmHttpModule(SwarmHttpModule &rhs) = delete;
  SwarmHttpModule operator=(SwarmHttpModule &rhs) = delete;
  SwarmHttpModule operator=(SwarmHttpModule &&rhs) = delete;

private:
  std::shared_ptr<SwarmNode> node_;
  const std::string          successString{"{\"response\": \"success\" }"};
  const std::string          failureString{
      "{\"response\": \"failure\", \"reason\": \"problems with parsing "
      "JSON!\"}"};
};
}  // namespace swarm
}  // namespace fetch
