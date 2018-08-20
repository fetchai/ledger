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

#include "ledger/chain/main_chain_controller.hpp"
#include "network/p2pservice/p2p_peer_details.hpp"
namespace fetch {
namespace chain {

class MainChainControllerProtocol : public service::Protocol
{
public:
  enum
  {
    CONNECT = 1,
    TRY_CONNECT,
    SHUTDOWN,
    START_SYNC,
    STOP_SYNC,
    INCOMING_PEERS,
    OUTGOING_PEERS
  };

  MainChainControllerProtocol(MainChainController *ctrl)
  {

    this->Expose(CONNECT, ctrl, &MainChainController::RPCConnect);
    this->Expose(TRY_CONNECT, ctrl, &MainChainController::TryConnect);

    this->Expose(SHUTDOWN, ctrl, &MainChainController::Shutdown);
    this->Expose(INCOMING_PEERS, ctrl, &MainChainController::IncomingPeers);
    this->Expose(OUTGOING_PEERS, ctrl, &MainChainController::OutgoingPeers);
  }
};

}  // namespace chain

}  // namespace fetch
