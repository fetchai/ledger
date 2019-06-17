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

#include "core/service_ids.hpp"
#include "ledger/protocols/dag_service.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/server.hpp"

namespace fetch {
namespace ledger {

DAGService::DAGService(MuddleEndpoint &muddle_endpoint, DAGPtr dag, Mode mode)
  : reactor_("DAGServiceReactor")
  , external_muddle_{muddle_endpoint}
  , dag_{dag}
{
  FETCH_UNUSED(mode);

  external_rpc_server_ = std::make_shared<Server>(external_muddle_, SERVICE_DAG, CHANNEL_RPC);

  dag_sync_protocol_ = std::make_shared<DAGSyncProtocol>(dag_);
  dag_sync_service_  = std::make_shared<DAGSyncService>(external_muddle_, dag_);

  // TX Sync protocol
  external_rpc_server_->Add(RPC_DAG_STORE_SYNC, dag_sync_protocol_.get());

  FETCH_LOG_INFO(LOGGING_NAME, "DAG initialised.");

  reactor_.Start();
}

DAGService::~DAGService()
{
  reactor_.Stop();
}

}  // namespace ledger
}  // namespace fetch
