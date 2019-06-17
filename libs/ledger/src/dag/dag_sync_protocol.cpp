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
  DAGNode getme;

  FETCH_LOG_WARN(LOGGING_NAME, "Fulfilling request for missing txs. Size:", missing_txs.size());

  uint64_t max_reply = 10;

  for(auto const &missing_hash : missing_txs)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "finding missing hash: ", missing_hash.ToBase64());

    if(dag_->GetDAGNode(missing_hash, getme))
    {
      ret.insert(getme);
    }

    if(max_reply-- == 0)
    {
      break;
    }
  }

#if 1
  for(auto const &node : ret)
  {
    FETCH_LOG_INFO(LOGGING_NAME, " returning a DAG hash: ", node.hash.ToBase64());
  }
#endif
  FETCH_LOG_WARN(LOGGING_NAME, "Fulfilled request for missing txs. Count: ", ret.size());

  return ret;
}
