#include "ledger/dag/dag_sync_protocol.hpp"

using namespace fetch::ledger;


DAGSyncProtocol::DAGSyncProtocol(std::shared_ptr<ledger::DAG>  dag)
  : dag_{dag}
{
  this->Expose(REQUEST_NODES, this, &Self::RequestNodes);
}

DAG::MissingNodes DAGSyncProtocol::RequestNodes(MissingTXs missing_txs)
{
  std::cerr << "whooo : DSP happened." << std::endl; // DELETEME_NH
  DAG::MissingNodes ret;
  DAGNode getme;

  for(auto const &missing_hash : missing_txs)
  {
    if(dag_->GetDAGNode(missing_hash, getme))
    {
      ret.push_back(getme);
    }
  }

  std::cerr << "returning: " << ret.size() << std::endl; // DELETEME_NH

  for(auto const &node : ret)
  {
    FETCH_LOG_INFO(LOGGING_NAME, " returning a hash: ", node.hash.ToBase64());
  }

  return ret;
}
