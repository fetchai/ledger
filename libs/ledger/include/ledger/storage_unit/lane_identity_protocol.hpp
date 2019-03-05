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

#include "ledger/storage_unit/lane_identity.hpp"
namespace fetch {
namespace ledger {

class LaneIdentityProtocol : public service::Protocol
{
public:
  enum
  {
    GET_IDENTITY = 1,
    GET_LANE_NUMBER,
    GET_TOTAL_LANES,
  };

  LaneIdentityProtocol(LaneIdentity &ctrl)
  {
    this->Expose(GET_IDENTITY, &ctrl, &LaneIdentity::Identity);
    this->Expose(GET_LANE_NUMBER, &ctrl, &LaneIdentity::GetLaneNumber);
    this->Expose(GET_TOTAL_LANES, &ctrl, &LaneIdentity::GetTotalLanes);
  }
};

}  // namespace ledger

}  // namespace fetch
