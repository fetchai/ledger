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

#include "muddle.hpp"
#include "muddle_register.hpp"
#include "muddle_registry.hpp"
#include "peer_list.hpp"
#include "peer_selector.hpp"

#include "muddle/muddle_status.hpp"
#include "variant/variant.hpp"

namespace fetch {
namespace muddle {
namespace {

void BuildDesiredPeers(PeerSelector const &peer_selector, variant::Variant &output)
{
  auto const desired = peer_selector.GetDesiredPeers();

  output = variant::Variant::Array(desired.size());

  std::size_t idx{0};
  for (auto const &address : desired)
  {
    output[idx++] = address.ToBase64();
  }
}

void BuildPeerInfo(PeerSelector const &peer_selector, variant::Variant &output)
{
  auto const peer_info = peer_selector.GetPeerCache();

  output = variant::Variant::Array(peer_info.size());

  std::size_t peer_idx{0};
  for (auto const &entry : peer_info)
  {
    auto &output_peer = output[peer_idx] = variant::Variant::Object();

    output_peer["currentIndex"] = entry.second.peer_index;

    auto &address_list = output_peer["addresses"] =
        variant::Variant::Array(entry.second.peer_data.size());

    std::size_t address_idx{0};
    for (auto const &address_entry : entry.second.peer_data)
    {
      auto &addr_entry = address_list[address_idx] = variant::Variant::Object();

      addr_entry["address"]     = address_entry.peer.ToString();
      addr_entry["unreachable"] = address_entry.unreachable;

      ++address_idx;
    }

    ++peer_idx;
  }
}

void BuildPeerSelection(PeerSelector const &peer_selector, variant::Variant &output)
{
  output = variant::Variant::Object();

  BuildDesiredPeers(peer_selector, output["desiredPeers"]);
  BuildPeerInfo(peer_selector, output["peerInfo"]);
}

void BuildPeerLists(PeerConnectionList const &peer_list, variant::Variant &output)
{
  auto const peers = peer_list.GetPersistentPeers();

  output = variant::Variant::Array(peers.size());

  std::size_t idx{0};
  for (auto const &peer : peers)
  {
    output[idx++] = peer.ToString();
  }
}

void BuildConnectionList(MuddleRegister const &reg, variant::Variant &output)
{
  auto const connections = reg.GetHandleIndex();

  output = variant::Variant::Array(connections.size());

  std::size_t idx{0};
  for (auto const &element : connections)
  {
    auto const &connection_info = *element.second;
    auto &      entry = output[idx] = variant::Variant::Object();

    entry["handle"]   = connection_info.handle;
    entry["address"]  = connection_info.address.ToBase64();
    entry["outgoing"] = connection_info.outgoing;

    ++idx;
  }
}

void BuildMuddleStatus(Muddle const &muddle, variant::Variant &output)
{
  output            = variant::Variant::Object();
  output["network"] = muddle.GetNetwork().ToString();
  output["address"] = muddle.GetAddress().ToBase64();

  BuildConnectionList(muddle.connection_register(), output["connections"]);
  BuildPeerLists(muddle.connection_list(), output["peers"]);
  BuildPeerSelection(muddle.peer_selector(), output["peerSelection"]);
}

}  // namespace

variant::Variant GetStatusSummary()
{
  auto instances = MuddleRegistry::Instance().GetMap();

  // create the output array
  auto output = variant::Variant::Array(instances.size());

  // build all the statuses for all the muddles
  std::size_t index{0};
  for (auto const &element : instances)
  {
    auto muddle = element.second.lock();
    if (muddle)
    {
      BuildMuddleStatus(*muddle, output[index]);
    }
    else
    {
      output[index] = variant::Variant::Null();
    }

    ++index;
  }

  return output;
}

}  // namespace muddle
}  // namespace fetch
