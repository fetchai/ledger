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

#include "core/assert.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "ledger/dag/dag.hpp"

#include <random>
#include <sstream>

namespace fetch {
namespace ledger {

class DAGHTTPInterface : public http::HTTPModule
{
public:
  using DAG   = ledger::DAG;
  static constexpr char const *LOGGING_NAME = "DAGHTTPInterface";

  DAGHTTPInterface(DAG &dag, DAGRpcService &rpc)
    : rng_{random_dev_()}
    , dag_(dag)
    , dag_rpc_(rpc)
  {
    Post("/api/dag/add-work",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return AddWork(params, request);
        });

    Post("/api/dag/add-bid",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return AddBid(params, request);
        }); 

    Get("/api/dag/status",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          return Status(params, request);
        });        

    certificate_.GenerateKeys();
  }

private:
  using Variant = variant::Variant;
  using ConstByteArray = byte_array::ConstByteArray;
  using ECDSASigner = crypto::ECDSASigner;
  using Rng = std::mt19937_64;
  using RngWord = Rng::result_type;


  // TODO (tfr): Temporary helper function that will be removed once wire format exists
  ECDSASigner certificate_;
  std::random_device random_dev_;
  Rng rng_;


  DAGNode GenerateNode(ConstByteArray const &data, uint64_t type)
  {
    // build up the DAG node
    DAGNode node;
    node.contents = data;
    node.identity = certificate_.identity();

    // TODO: Set previous and type
    auto prev_candidates = dag_.last_nodes();

    if(prev_candidates.size() > 0 )
    {
      node.previous.push_back( prev_candidates[ rng_() % prev_candidates.size() ]);
    }

    if(prev_candidates.size() > 3 )
    {
      node.previous.push_back( prev_candidates[ rng_() % prev_candidates.size() ]);
    }

    node.Finalise();

    if(!certificate_.Sign(node.hash))
    {
      throw std::runtime_error("Signing failed");
    }

    node.signature = certificate_.signature();

    return node;
  }
  // TODO(tfr): end of temporary function

  http::HTTPResponse AddWork(http::ViewParameters const & /*params*/,
                             http::HTTPRequest const &request)
  {
    Variant response     = Variant::Object();

    json::JSONDocument doc;
    doc.Parse(request.body());

    if(!doc.Has("payload"))
    {
      response["error"] = "Work request did not have a payload.";
      return http::CreateJsonResponse(response);
    }

    // TODO(tfr): Use wire format.
    auto payload = doc["payload"].As<ConstByteArray>();
    DAGNode node = GenerateNode(payload, DAGNode::WORK);

    std::cout << "================================" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << node.contents << std::endl;
    std::cout << "================================" << std::endl;    
    std::cout << "================================" << std::endl;

    dag_rpc_.BroadcastDAGNode(node);
    dag_.Push(node);

    return http::CreateJsonResponse(response);
  }

  http::HTTPResponse AddBid(http::ViewParameters const & /*params*/,
                            http::HTTPRequest const &request)
  {
    Variant response     = Variant::Object();
/*
    dag_rpc_.BroadcastDAGNode(node);
    dag_.Push(node);
*/
    return http::CreateJsonResponse(response);
  }


  http::HTTPResponse Status(http::ViewParameters const & /*params*/,
                            http::HTTPRequest const &request)
  {
    Variant response     = Variant::Object();
/*
    dag_rpc_.BroadcastDAGNode(node);
    dag_.Push(node);
*/
    return http::CreateJsonResponse(response);
  }

  DAG &  dag_;
  DAGRpcService & dag_rpc_;
};



}
}