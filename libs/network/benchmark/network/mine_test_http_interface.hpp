#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "core/logger.hpp"
#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"
#include "http/server.hpp"
#include "network_classes.hpp"
#include "variant/variant.hpp"

namespace fetch {
namespace network_mine_test {

template <typename T>
class HttpInterface : public fetch::http::HTTPModule
{
public:
  explicit HttpInterface(std::shared_ptr<T> node)
    : node_{node}
  {
    AttachPages();
  }

  void AttachPages()
  {
    LOG_STACK_TRACE_POINT;
    HTTPModule::Post("/add-endpoint",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->AddEndpoint(params, req);
                     });

    HTTPModule::Post("/start",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->Start(params, req);
                     });

    HTTPModule::Post("/stop",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->Stop(params, req);
                     });

    HTTPModule::Post("/reset",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->Reset(params, req);
                     });

    HTTPModule::Post("/mainchain",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->Mainchain(params, req);
                     });

    // HTTPModule::Post("/allchain",
    //                 [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
    //                   return this->AllChain(params, req);
    //                 });
  }

  HttpInterface(HttpInterface &&rhs)
  {
    LOG_STACK_TRACE_POINT;
    node_ = std::move(rhs.node());
    AttachPages();
  }

  http::HTTPResponse AddEndpoint(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);

    LOG_STACK_TRACE_POINT;
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      network_benchmark::Endpoint endpoint(doc);

      node_->AddEndpoint(endpoint);

      return http::HTTPResponse(successString);
    }
    catch (...)
    {
      return http::HTTPResponse(failureString);
    }
  }

  http::HTTPResponse Start(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    LOG_STACK_TRACE_POINT;

    node_->startMining();
    return http::HTTPResponse(successString);
  }

  http::HTTPResponse Stop(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    LOG_STACK_TRACE_POINT;

    node_->stopMining();
    return http::HTTPResponse(successString);
  }

  http::HTTPResponse Reset(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    node_->reset();
    return http::HTTPResponse(successString);
  }

  http::HTTPResponse Mainchain(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    auto chainArray = node_->HeaviestChain();

    variant::Variant result = variant::Variant::Array(chainArray.size());

    std::size_t index = 0;
    for (auto &i : chainArray)
    {

      variant::Variant temp = variant::Variant::Object();
      temp["blockNumber"]   = i.body.block_number;
      temp["hashcurrent"]   = ToHex(i.body.hash);
      temp["hashprev"]      = ToHex(i.body.previous_hash);

      result[index++] = temp;
    }

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  // http::HTTPResponse AllChain(http::ViewParameters const &params, http::HTTPRequest const &req)
  //{
  //  auto chainArray = node_->AllChain();

  //  auto heaviestBlock = chainArray.first;
  //  auto chainArrays   = chainArray.second;

  //  variant::Variant result = variant::Variant::Object();

  //  {
  //    variant::Variant temp = variant::Variant::Object();
  //    temp["minerNumber"]  = heaviestBlock.body().miner_number;
  //    temp["blockNumber"]  = heaviestBlock.body().block_number;
  //    temp["hashcurrent"]  = ToHex(heaviestBlock.hash());
  //    temp["hashprev"]     = ToHex(heaviestBlock.body().previous_hash);

  //    result["heaviest"] = temp;
  //  }

  //  // We now have an array of arrays
  //  variant::Variant arrays = variant::Variant::Array(chainArray.second.size());

  //  std::size_t i = 0;
  //  std::size_t j = 0;
  //  for (auto &chain : chainArray.second)
  //  {
  //    variant::Variant chainVar = variant::Variant::Array(chain.size());

  //    for (auto &block : chain)
  //    {
  //      variant::Variant temp = variant::Variant::Object();
  //      temp["minerNumber"]  = block.body().miner_number;
  //      temp["blockNumber"]  = block.body().block_number;
  //      temp["hashcurrent"]  = ToHex(block.hash());
  //      temp["hashprev"]     = ToHex(block.body().previous_hash);

  //      chainVar[j++] = temp;
  //    }

  //    arrays[i++] = chainVar;
  //    j           = 0;
  //  }

  //  result["chains"] = arrays;

  //  std::ostringstream ret;
  //  ret << result;

  //  return http::HTTPResponse(ret.str());
  //}

  const std::shared_ptr<T> &node() const
  {
    return node_;
  };

private:
  std::shared_ptr<T> node_;
  const std::string  successString{"{\"response\": \"success\" }"};
  const std::string  failureString{
      "{\"response\": \"failure\", \"reason\": \"problems with parsing "
      "JSON!\"}"};
};

}  // namespace network_mine_test
}  // namespace fetch
