#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "ledger/storage_unit/lane_controller.hpp"
namespace fetch {
namespace ledger {

class LaneControllerProtocol : public service::Protocol
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

  LaneControllerProtocol(LaneController *ctrl)
  {

    this->Expose(CONNECT, ctrl, &LaneController::RPCConnect);
    this->Expose(TRY_CONNECT, ctrl, &LaneController::TryConnect);

    this->Expose(SHUTDOWN, ctrl, &LaneController::Shutdown);
    this->Expose(INCOMING_PEERS, ctrl, &LaneController::IncomingPeers);
    this->Expose(OUTGOING_PEERS, ctrl, &LaneController::OutgoingPeers);
  }
};

}  // namespace ledger

}  // namespace fetch
