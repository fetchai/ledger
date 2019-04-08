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

#include "core/serializers/stl_types.hpp"
#include "core/service_ids.hpp"
#include "ledger/dag/dag.hpp"
#include "network/service/protocol.hpp"
#include "core/state_machine.hpp"

namespace fetch
{
namespace ledger 
{

class DAGProtocol : public fetch::service::Protocol
{
public:
  using NodeList        = std::vector<DAGNode>;

  enum // TODO: move to service ids
  {
    NUMBER_OF_DAG_NODES = 723,
    DOWNLOAD_DAG
  };

  enum 
  {
    MAX_DAG_CHUNK_SIZE = 100000
  };


  explicit DAGProtocol(DAG &dag) 
  : dag_(dag)
  {
    Expose(NUMBER_OF_DAG_NODES, this, &DAGProtocol::NumberOfDAGNodes);
    Expose(DOWNLOAD_DAG, this, &DAGProtocol::DownloadDAG);
  }

private:
  /// @name DAG Protocol Functions
  /// @{
  uint64_t NumberOfDAGNodes()
  {
    LOG_STACK_TRACE_POINT;
    return dag_.node_count();
  }

  NodeList DownloadDAG(uint64_t const &part, uint64_t chunk_size) 
  {
    LOG_STACK_TRACE_POINT;

    if(chunk_size > MAX_DAG_CHUNK_SIZE)
    {
      return NodeList{};
    }

    uint64_t             from = part * chunk_size;
    uint64_t             to   = from + chunk_size;

    if (to > dag_.node_count())
    {
      to = dag_.node_count();
    }

    FETCH_LOG_INFO("DAGProtocol", "Getting ", from, " to ", to, " / ", dag_.node_count());
    return dag_.GetChunk(from, to);
  }
  /// @}

  DAG &dag_;
};  


}
}