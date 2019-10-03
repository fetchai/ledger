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

/**
 * For a given set of peer selector address, build the JSON representation
 *
 * @param address_set The input address set to convert
 * @param output The output variant structure
 */
void BuildPeerSet(PeerSelector::Addresses const &address_set, variant::Variant &output)
{
  output = variant::Variant::Array(address_set.size());

  std::size_t idx{0};
  for (auto const &address : address_set)
  {
    output[idx++] = address.ToBase64();
  }
}

/**
 * Build the JSON representation of the PeerSelectors Peer Cache
 *
 * @param peer_selector The input peer selector
 * @param output The output variant object
 */
void BuildPeerInfo(PeerSelector const &peer_selector, variant::Variant &output)
{
  auto const peer_info = peer_selector.GetPeerCache();

  output = variant::Variant::Array(peer_info.size());

  std::size_t peer_idx{0};
  for (auto const &entry : peer_info)
  {
    auto &output_peer = output[peer_idx] = variant::Variant::Object();

    output_peer["targetAddress"] = entry.first.ToBase64();
    output_peer["currentIndex"]  = entry.second.peer_index;

    auto &address_list = output_peer["addresses"] =
        variant::Variant::Array(entry.second.peer_data.size());

    std::size_t address_idx{0};
    for (auto const &address_entry : entry.second.peer_data)
    {
      auto &addr_entry = address_list[address_idx] = variant::Variant::Object();

      addr_entry["peerAddress"] = address_entry.peer.ToString();
      addr_entry["unreachable"] = address_entry.unreachable;

      ++address_idx;
    }

    ++peer_idx;
  }
}

/**
 * Build the JSON representation of a PeerSelectors internal status
 *
 * @param peer_selector The input peer selector
 * @param output The output variant object
 */
void BuildPeerSelection(PeerSelector const &peer_selector, variant::Variant &output)
{
  output = variant::Variant::Object();

  BuildPeerSet(peer_selector.GetDesiredPeers(), output["desiredPeers"]);
  BuildPeerSet(peer_selector.GetKademliaPeers(), output["kademliaPeers"]);
  BuildPeerInfo(peer_selector, output["peerInfo"]);
}

/**
 * Build the JSON representation of the PeerConnectionList
 *
 * @param peer_list The input peer connection list
 * @param output The output variant object
 */
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

/**
 * Build the JSON representation of the Muddle Registers connection list
 *
 * @param reg The input register
 * @param output The output variant object
 */
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

/**
 * Build the JSON representation for the status of a given muddle
 *
 * @param muddle The muddle to evaluate
 * @param output The output variant object
 */
void BuildMuddleStatus(Muddle const &muddle, variant::Variant &output)
{
  output            = variant::Variant::Object();
  output["network"] = muddle.GetNetwork().ToString();
  output["address"] = muddle.GetAddress().ToBase64();

  BuildConnectionList(muddle.connection_register(), output["connections"]);
  BuildPeerLists(muddle.connection_list(), output["peers"]);
  BuildPeerSelection(muddle.peer_selector(), output["peerSelection"]);
}

/**
 * Filter a given muddle registry map for networks matching a specified
 *
 * @param map The map of muddle instances that need to be filtered
 * @param target_network The target network to be extracted
 * @return THe filtered map of muddle instances
 */
MuddleRegistry::MuddleMap FilterInstances(MuddleRegistry::MuddleMap const &map,
                                          std::string const &              target_network)
{
  MuddleRegistry::MuddleMap filtered_map{};

  for (auto const &instance : map)
  {
    auto const network = instance.second.lock();
    if (network)
    {
      auto const network_name = network->GetNetwork().ToString();

      if (target_network.empty() || (network_name == target_network))
      {
        filtered_map.emplace(instance.first, instance.second);
      }
    }
  }

  return filtered_map;
}

/**
 * Get the filtered set of muddle instances based on the specified filter
 *
 * @param target_network The target network to filter for (blank means no filtering)
 * @return The filtered set of muddle instances
 */
MuddleRegistry::MuddleMap GetFilterInstances(std::string const &target_network)
{
  // get the complete set of instance
  auto instances = MuddleRegistry::Instance().GetMap();

  return FilterInstances(instances, target_network);
}

}  // namespace

/**
 * Generate the JSON status summary for the muddle instances on the system
 *
 * @param network The network name to filter from
 * @return The generated JSON response
 */
variant::Variant GetStatusSummary(std::string const &network)
{
  auto instances = GetFilterInstances(network);

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
