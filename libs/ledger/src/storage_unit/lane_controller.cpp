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

#include "core/containers/set_difference.hpp"
#include "ledger/storage_unit/lane_controller.hpp"
#include "muddle/muddle_interface.hpp"

namespace fetch {
namespace ledger {

LaneController::LaneController(muddle::MuddleInterface &muddle)
  : muddle_(muddle)
{}

void LaneController::UseThesePeers(AddressMap const &addresses)
{
  muddle_.ConnectTo(addresses);
  muddle_.DisconnectFrom(muddle_.GetRequestedPeers() - addresses);
}

}  // namespace ledger
}  // namespace fetch
