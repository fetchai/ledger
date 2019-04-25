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
  using Digest          = DAGInterface::Digest;
  using NodeArray       = DAGInterface::NodeArray;
  using NodeDeque       = DAGInterface::NodeDeque;  
  using DigestArray     = DAGInterface::DigestArray;

  enum 
  {
    GET_LATEST        = 1,
    GET_NODES_BEFORE  = 2
  };

  explicit DAGProtocol(DAGInterface &dag) 
  : dag_(dag)
  {
    Expose(GET_LATEST,       this, &DAGProtocol::GetLatest);
    Expose(GET_NODES_BEFORE, this, &DAGProtocol::GetNodesBefore);
  }

private:
  /// @name DAG Protocol Functions
  /// @{
  NodeArray GetLatest()
  {
    auto latest = dag_.GetLatest();

    NodeArray ret;
    for(auto &l: latest)
    {
      ret.push_back(l);
    }

    return ret;
  }

  NodeArray GetNodesBefore(DigestArray hashes, uint64_t const &block_number, uint64_t const &count)
  {
    return dag_.GetBefore(hashes, block_number, count);
  }
  /// @}

  DAGInterface &dag_;
};  


}
}