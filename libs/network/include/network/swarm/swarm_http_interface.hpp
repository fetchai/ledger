#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "http/server.hpp"
#include "network/swarm/swarm_node.hpp"
#include <iostream>
#include <utility>

namespace fetch {
namespace swarm {

class SwarmHttpModule : public fetch::http::HTTPModule
{
public:
  explicit SwarmHttpModule(std::shared_ptr<SwarmNode> node)
    : node_{std::move(node)}
  {
    AttachPages();
  }

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
      return fetch::http::HTTPResponse(failureString_);
    }
  }

  SwarmHttpModule(SwarmHttpModule &rhs) = delete;
  SwarmHttpModule operator=(SwarmHttpModule &rhs) = delete;
  SwarmHttpModule operator=(SwarmHttpModule &&rhs) = delete;

private:
  std::shared_ptr<SwarmNode> node_;
  const std::string          successString_{"{\"response\": \"success\" }"};
  const std::string          failureString_{
      "{\"response\": \"failure\", \"reason\": \"problems with parsing "
      "JSON!\"}"};
};
}  // namespace swarm
}  // namespace fetch
