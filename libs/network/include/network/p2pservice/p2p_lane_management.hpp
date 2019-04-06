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

#include "network/uri.hpp"

#include <cstdint>
#include <unordered_set>

namespace fetch {
namespace p2p {

class LaneManagement
{
public:
  using LaneIndex = uint32_t;
  using Uri       = network::Uri;
  using UriSet    = std::unordered_set<Uri>;

  // Construction / Destruction
  LaneManagement()          = default;
  virtual ~LaneManagement() = default;

  /// @name Lane Management
  /// @{
  virtual void     UseThesePeers(LaneIndex lane, UriSet const &uris) = 0;
  virtual void     Shutdown(LaneIndex lane)                          = 0;
  virtual uint32_t GetLaneNumber(LaneIndex lane)                     = 0;
  virtual int      IncomingPeers(LaneIndex lane)                     = 0;
  virtual int      OutgoingPeers(LaneIndex lane)                     = 0;
  virtual bool     IsAlive(LaneIndex lane)                           = 0;
  /// @}
};

}  // namespace p2p
}  // namespace fetch
