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

#include "core/byte_array/const_byte_array.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rbc.hpp"
#include "network/uri.hpp"

#include <unordered_map>

namespace fetch {
namespace dkg {

class PreDkgSync
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using MuddleAddress  = ConstByteArray;
  using PeersList      = std::unordered_map<MuddleAddress, fetch::network::Uri>;

  PreDkgSync(muddle::MuddleInterface &muddle, uint16_t channel);
  void ResetCabinet(PeersList const &peers);
  void Connect();
  bool ready();

private:
  using Cabinet = std::set<MuddleAddress>;

  muddle::MuddleInterface &         muddle_;
  PeersList                         peers_;
  Cabinet                           cabinet_;
  muddle::RBC                       rbc_;
  std::mutex                        mutex_;
  std::unordered_set<MuddleAddress> joined_;
  uint32_t                          joined_counter_{0};
  bool                              committee_sent_{false};
  bool                              ready_{false};

  void OnRbcMessage(MuddleAddress const &from, std::set<MuddleAddress> const &connections);
  bool CheckConnections(std::set<MuddleAddress> const &connections);
  void ReceivedDkgReady();
};
}  // namespace dkg
}  // namespace fetch
