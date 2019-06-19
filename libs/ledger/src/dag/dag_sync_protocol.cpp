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

#include "ledger/dag/dag_sync_protocol.hpp"

using namespace fetch::ledger;

DAGSyncProtocol::DAGSyncProtocol(std::shared_ptr<ledger::DAGInterface> dag)
  : dag_{std::move(dag)}
{
  this->Expose(REQUEST_NODES, this, &Self::RequestNodes);
}

DAG::MissingNodes DAGSyncProtocol::RequestNodes(MissingTXs missing_txs)
{
  DAG::MissingNodes ret;
  DAGNode           getme;

  FETCH_LOG_WARN(LOGGING_NAME, "Fulfilling request for missing txs. Size:", missing_txs.size());

  uint64_t max_reply = 10;

  for (auto const &missing_hash : missing_txs)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "finding missing hash: ", missing_hash.ToBase64());

    if (dag_->GetDAGNode(missing_hash, getme))
    {
      ret.insert(getme);
    }

    if (max_reply-- == 0)
    {
      break;
    }
  }

  return ret;
}
