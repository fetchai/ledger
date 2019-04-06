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
#include "network/muddle/muddle.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace fetch {
namespace ledger {

class LaneController
{
public:
  using Muddle     = muddle::Muddle;
  using MuddlePtr  = std::shared_ptr<Muddle>;
  using Uri        = Muddle::Uri;
  using UriSet     = std::unordered_set<Uri>;
  using Address    = Muddle::Address;
  using AddressSet = std::unordered_set<Address>;

  static constexpr char const *LOGGING_NAME = "LaneController";

  // Construction / Destruction
  LaneController(std::weak_ptr<LaneIdentity> identity, MuddlePtr muddle);
  LaneController(LaneController const &) = delete;
  LaneController(LaneController &&)      = delete;
  ~LaneController()                      = default;

  /// External controls
  /// @{
  void RPCConnectToURIs(const std::vector<Uri> &uris);
  void Shutdown();
  void StartSync();
  void StopSync();
  /// @}

  /// Internal controls
  /// @{
  void WorkCycle();
  void UseThesePeers(UriSet uris);

  AddressSet GetPeers();
  void       GeneratePeerDeltas(UriSet &create, UriSet &remove);
  /// @}

  // Operators
  LaneController &operator=(LaneController const &) = delete;
  LaneController &operator=(LaneController &&) = delete;

private:
  std::weak_ptr<LaneIdentity> lane_identity_;

  // Most methods do not need both mutexes. If they do, they should
  // acquire them in alphabetic order

  mutex::Mutex services_mutex_{__LINE__, __FILE__};
  mutex::Mutex desired_connections_mutex_{__LINE__, __FILE__};

  std::unordered_map<Uri, Address> peer_connections_;
  UriSet                           desired_connections_;

  MuddlePtr muddle_;
};

}  // namespace ledger
}  // namespace fetch
