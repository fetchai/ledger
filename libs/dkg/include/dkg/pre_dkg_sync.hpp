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
#include "dkg/rbc.hpp"
#include "network/muddle/muddle.hpp"
/*#include "telemetry/registry.hpp"*/
#include "telemetry/telemetry.hpp"
#include "telemetry/gauge.hpp"

#include <unordered_map>

namespace fetch {
namespace dkg {

class PreDkgSync
{
public:
  using Endpoint       = muddle::MuddleEndpoint;
  using ConstByteArray = byte_array::ConstByteArray;
  using MuddleAddress  = ConstByteArray;
  using CabinetMembers = std::set<MuddleAddress>;

  PreDkgSync(Endpoint &endpoint, ConstByteArray address, uint8_t channel);

  void ResetCabinet(CabinetMembers const &members, uint32_t threshold);
  bool Ready();

private:
  using Cabinet = std::set<MuddleAddress>;

  Endpoint         &endpoint_;
  CabinetMembers members_;
  Cabinet        cabinet_;
  RBC            rbc_;
  std::mutex     mutex_;
  uint32_t       ready_peers_ = 0;
  bool           self_ready_ = false;
  uint32_t       threshold_ = std::numeric_limits<uint32_t>::max();

  std::map<MuddleAddress, std::unordered_set<MuddleAddress>> other_peer_connections;

  const uint64_t time_quantisation_s     = 30;
  const uint64_t grace_period_time_units = 3;

  uint64_t start_time_ = std::numeric_limits<uint64_t>::max();

  void OnRbcMessage(MuddleAddress const &from, std::set<MuddleAddress> const &connections);
  bool CheckConnections(MuddleAddress const &from, std::set<MuddleAddress> const &connections);
  void SetStartTime();

  /*telemetry::CounterPtr         tele_ready_peers_;*/
  /* telemetry::GaugePtr<uint64_t> sync_time_gauge_;*/
};
}  // namespace dkg
}  // namespace fetch
