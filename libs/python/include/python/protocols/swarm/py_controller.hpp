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

#include "protocols/swarm/controller.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace protocols {

void BuildChainController(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ChainController>(module, "ChainController")
      .def(py::init<>())
      .def("block_count", &ChainController::block_count)
      .def("AddBulkBlocks", &ChainController::AddBulkBlocks)
      .def("PushBlock", &ChainController::PushBlock)
      .def("with_blocks_do", (void (ChainController::*)(
                                 std::function<void(ChainManager::shared_block_type,
                                                    const ChainManager::chain_map_type &)>) const) &
                                 ChainController::with_blocks_do)
      .def("with_blocks_do",
           (void (ChainController::*)(std::function<void(ChainManager::shared_block_type,
                                                         ChainManager::chain_map_type &)>)) &
               ChainController::with_blocks_do)
      .def("GetLatestBlocks", &ChainController::GetLatestBlocks)
      .def("GetNextBlock", &ChainController::GetNextBlock)
      .def("AddBulkSummaries", &ChainController::AddBulkSummaries)
      .def("SetGroupParameter", &ChainController::SetGroupParameter);
}

void BuildSwarmController(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<SwarmController, fetch::protocols::ChainController,
             fetch::service::HasPublicationFeed>(module, "SwarmController")
      .def(py::init<const uint64_t &, network::NetworkManager *,
                    fetch::protocols::SharedNodeDetails &>())
      .def("SetClientIPCallback", &SwarmController::SetClientIPCallback)
      .def("IncreaseGroupingParameter", &SwarmController::IncreaseGroupingParameter)
      .def("Connect", &SwarmController::Connect)
      .def("Bootstrap", &SwarmController::Bootstrap)
      .def("SuggestPeers", &SwarmController::SuggestPeers)
      //    .def("with_peers_do", ( void (SwarmController::*)(std::function<void
      //    (int)>) ) &SwarmController::with_peers_do) .def("with_peers_do", (
      //    void (SwarmController::*)(std::function<void (int, int &)>) )
      //    &SwarmController::with_peers_do)
      .def("with_node_details", &SwarmController::with_node_details)
      .def("need_more_connections", &SwarmController::need_more_connections)
      .def("Ping", &SwarmController::Ping)
      .def("GetAddress", &SwarmController::GetAddress)
      .def("with_suggestions_do", &SwarmController::with_suggestions_do)
      .def("GetGroupingParameter", &SwarmController::GetGroupingParameter)
      //    .def("with_shards_do", ( void
      //    (SwarmController::*)(std::function<void (const int &, int &)>) )
      //    &SwarmController::with_shards_do) .def("with_shards_do", ( void
      //    (SwarmController::*)(std::function<void (const int &)>) )
      //    &SwarmController::with_shards_do)
      .def("EnoughPeerConnections", &SwarmController::EnoughPeerConnections)
      .def("ConnectChainKeeper", &SwarmController::ConnectChainKeeper)
      .def("with_server_details_do", &SwarmController::with_server_details_do)
      .def("with_client_details_do", &SwarmController::with_client_details_do)
      .def("with_shard_details_do", &SwarmController::with_shard_details_do)
      .def("RequestPeerConnections", &SwarmController::RequestPeerConnections)
      .def("Hello", &SwarmController::Hello)
      .def("SetGroupParameter", &SwarmController::SetGroupParameter);
}
};  // namespace protocols
};  // namespace fetch
