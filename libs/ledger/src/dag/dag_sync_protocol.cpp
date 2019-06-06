#include "ledger/dag/dag_sync_protocol.hpp"

using namespace fetch::ledger;


DAGSyncProtocol::DAGSyncProtocol(std::shared_ptr<ledger::DAG> dag)
  : dag_{std::move(dag)}
{
  this->Expose(REQUEST_NODES, this, &Self::RequestNodes);
}

DAG::MissingNodes DAGSyncProtocol::RequestNodes(MissingTXs missing_txs)
{
  DAG::MissingNodes ret;
  DAGNode getme;

  for(auto const &missing_hash : missing_txs)
  {
    if(dag_->GetDAGNode(missing_hash, getme))
    {
      ret.push_back(getme);
    }
  }

#if 0
  for(auto const &node : ret)
  {
    FETCH_LOG_INFO(LOGGING_NAME, " returning a DAG hash: ", node.hash.ToBase64());
  }
#endif

  return ret;
}
