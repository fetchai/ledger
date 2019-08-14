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

#include "dkg/pre_dkg_sync.hpp"

#include "telemetry/registry.hpp"

#include <cstddef>
#include <cstdint>
#include <ctime>

char const *LOGGING_NAME = "Pre-Dkg Sync";

namespace fetch {
namespace dkg {

PreDkgSync::PreDkgSync(Endpoint &endpoint, ConstByteArray address, uint8_t channel)
  : endpoint_{endpoint}
  , rbc_{endpoint_, address,
         [this](MuddleAddress const &address, ConstByteArray const &payload) -> void {
           std::set<MuddleAddress>               connections;
           fetch::serializers::MsgPackSerializer serializer{payload};
           serializer >> connections;

           OnRbcMessage(address, connections);
         },
         channel}
  , sync_time_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "sync_time_gague", "Time the DKG should start")}
  , tele_ready_peers_{telemetry::Registry::Instance().CreateCounter(
        "ready_peers_total", "The total number of ready peers (pre dkg sync)")}
// note, counters must end in '_total'.
{}

void PreDkgSync::OnRbcMessage(MuddleAddress const &from, std::set<MuddleAddress> const &connections)
{
  // Check peer is valid
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (members_.find(from) == members_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Message from ", from, " discard as not in members!");
      return;
    }
  }

  // Check peer's connections are valid
  if (!CheckConnections(from, connections))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Message from ", from,
                   " discarded - provided incorrect connections!");
  }

  // If these conditions are good, add the info to the map
  {
    std::unique_lock<std::mutex>       lock(mutex_);
    std::unordered_set<MuddleAddress> &peer_connections = other_peer_connections[from];

    for (auto const &connection : connections)
    {
      peer_connections.insert(connection);
    }

    ready_peers_++;
    tele_ready_peers_->add(1);
  }

  SetStartTime();
}

void PreDkgSync::SetStartTime()
{
  // Check if this meets the minimum requirements to set a point to start.
  // Start time is as soon as there are enough ready peers, plus N time units
  std::unique_lock<std::mutex> lock(mutex_);
  if (start_time_ == std::numeric_limits<uint64_t>::max() && ready_peers_ >= threshold_)
  {
    uint64_t current_time = static_cast<uint64_t>(std::time(nullptr));
    start_time_ =
        (((current_time / time_quantisation_s) + grace_period_time_units) * time_quantisation_s);
    sync_time_gauge_->set(start_time_);
    assert(start_time_ > current_time);

    FETCH_LOG_INFO(LOGGING_NAME, "The conditions for starting DKG are met. Start: ", start_time_,
                   " time until: ", start_time_ - current_time);
  }
}

bool PreDkgSync::CheckConnections(MuddleAddress const &          from,
                                  std::set<MuddleAddress> const &connections)
{
  std::lock_guard<std::mutex> lock(mutex_);

  if (connections.size() + 1 < threshold_)
  {
    FETCH_LOG_WARN(
        LOGGING_NAME,
        "Peer indicated it is ready but with less connections than the threshold. Connections: ",
        connections.size() + 1);
    return false;
  }

  if (connections.find(from) != connections.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Peer provided a connection list with itself as a member which is invalid");
    return false;
  }

  for (auto const &id : connections)
  {
    if (members_.find(id) == members_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Message contains node ", id, " not in members list");
      return false;
    }
  }
  return true;
}

void PreDkgSync::ResetCabinet(CabinetMembers const &members, uint32_t threshold)
{
  std::lock_guard<std::mutex> lock(mutex_);
  members_   = members;
  threshold_ = threshold;
  rbc_.ResetCabinet(members);
}

bool PreDkgSync::Ready()
{
  // if we meet the conditions for syncing for the first time
  if (!self_ready_ && endpoint_.GetDirectlyConnectedPeers().size() + 1 >= threshold_)
  {
    self_ready_ = true;

    FETCH_LOG_INFO(LOGGING_NAME, "*** Node is ready ***");

    // Send ready message to other members
    fetch::serializers::MsgPackSerializer serializer;
    serializer << endpoint_.GetDirectlyConnectedPeers();
    rbc_.SendRBroadcast(serializer.data());
    SetStartTime();
  }

  uint64_t current_time = static_cast<uint64_t>(std::time(nullptr));

  std::lock_guard<std::mutex> lock(mutex_);
  return start_time_ <= current_time;
}

void PreDkgSync::SetTimeQuantisation(uint64_t time)
{
  time_quantisation_s = time;
}

}  // namespace dkg
}  // namespace fetch
