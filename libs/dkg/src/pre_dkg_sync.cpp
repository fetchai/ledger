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

#include <thread>
#include <chrono>

char const *LOGGING_NAME = "Pre-Dkg Sync";

namespace fetch {
namespace dkg {

PreDkgSync::PreDkgSync(muddle::MuddleInterface &muddle, uint16_t channel)
  : muddle_{muddle}
  , rbc_{muddle_.GetEndpoint(), muddle_.GetAddress(),
         [this](MuddleAddress const &address, ConstByteArray const &payload) -> void {
           std::set<MuddleAddress>               connections;
           fetch::serializers::MsgPackSerializer serializer{payload};
           serializer >> connections;
           OnRbcMessage(address, connections);
         },
         channel}
{}

void PreDkgSync::OnRbcMessage(MuddleAddress const &from, std::set<MuddleAddress> const &connections)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Received connections message");
  if (peers_.find(from) == peers_.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Message from ", from, " discard as not in peers");
    return;
  }
  if (CheckConnections(connections))
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (joined_.find(from) == joined_.end())
    {
      joined_.insert(from);
      ++joined_counter_;
      lock.unlock();
      ReceivedDkgReady();
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Message from ", from, " discard as duplicate message");
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Message from ", from, " discard as different connections");
  }
}

bool PreDkgSync::CheckConnections(std::set<MuddleAddress> const &connections)
{
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto const &id : connections)
  {
    if (peers_.find(id) == peers_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Message contains node ", id, " not in peers list");
      return false;
    }
  }
  return true;
}

void PreDkgSync::ReceivedDkgReady()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (committee_sent_ && joined_counter_ == peers_.size() - 1)
  {
    ready_ = true;
  }
}

void PreDkgSync::ResetCabinet(PeersList const &peers)
{
  peers_ = peers;

  cabinet_.clear();  // TODO(tfr): Check with Jenny.
  for (auto const &peer : peers_)
  {
    cabinet_.insert(peer.first);
  }
  assert(cabinet_.size() == peers_.size());
  rbc_.ResetCabinet(cabinet_);
}

void PreDkgSync::Connect()
{
  for (auto const &peer : peers_)
  {
    muddle_.ConnectTo(peer.first, peer.second);
  }

  while (muddle_.GetDirectlyConnectedPeers().size() != peers_.size() - 1)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  // Send ready message with list of connected peers
  fetch::serializers::MsgPackSerializer serializer;
  serializer << cabinet_;
  rbc_.Broadcast(serializer.data());
  committee_sent_ = true;
  ReceivedDkgReady();
}

bool PreDkgSync::ready()
{
  std::lock_guard<std::mutex> lock(mutex_);
  return ready_;
}
}  // namespace dkg
}  // namespace fetch
