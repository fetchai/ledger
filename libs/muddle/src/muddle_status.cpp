//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "kademlia/peer_tracker.hpp"
#include "muddle.hpp"
#include "muddle_register.hpp"
#include "muddle_registry.hpp"
#include "peer_list.hpp"

#include "core/string/trim.hpp"
#include "muddle/muddle_status.hpp"
#include "variant/variant.hpp"

#include <chrono>
#include <ctime>

namespace fetch {
namespace muddle {
namespace {

/**
 * For a given set of peer selector address, build the JSON representation
 *
 * @param address_set The input address set to convert
 * @param output The output variant structure
 */
void BuildPeerSet(Muddle::Addresses const &address_set, variant::Variant &output)
{
  output = variant::Variant::Array(address_set.size());

  std::size_t idx{0};
  for (auto const &address : address_set)
  {
    output[idx++] = address.ToBase64();
  }
}

void BuildConnectionPriorities(PeerTracker const &peer_tracker, variant::Variant &output)
{
  auto                         priorities_map = peer_tracker.connection_priority();
  std::vector<AddressPriority> priorities;
  priorities.reserve(priorities_map.size());
  for (auto &p : priorities_map)
  {
    priorities.emplace_back(p.second);
  }

  output = variant::Variant::Array(priorities.size());

  // Updating priorities
  for (auto &p : priorities)
  {
    p.UpdatePriority();
  }

  std::sort(priorities.begin(), priorities.end(), [](auto const &a, auto const &b) -> bool {
    // Making highest priority appear first
    return a.priority > b.priority;
  });

  std::size_t       idx{0};
  std::stringstream ss;
  for (auto const &entry : priorities)
  {
    auto &output_peer = output[idx] = variant::Variant::Object();
    output_peer["address"]          = entry.address.ToBase64();
    output_peer["priority"]         = entry.priority;
    output_peer["persistent"]       = entry.persistent;
    output_peer["bucket"]           = entry.bucket;
    output_peer["connection_value"] = entry.connection_value;
    output_peer["is_connected"]     = entry.is_connected;
    /*
    // TODO(tfr): Figure out how to convert steady clock to system clock
        std::time_t t1 = std::chrono::system_clock::to_time_t(entry.connected_since);

        auto ts1 = static_cast<std::string>(std::ctime(&t1));
        string::Trim(ts1);
        output_peer["connected_since"] = ts1;

        std::time_t t2  = std::chrono::system_clock::to_time_t(entry.desired_expiry);
        auto        ts2 = static_cast<std::string>(std::ctime(&t2));
        string::Trim(ts2);

        output_peer["desired_expiry"] = ts2;
    */
    ++idx;
  }
}

/**
 * Build the JSON representation of a PeerTracker internal status
 * // TODO
 * @param peer_selector The input peer selector
 * @param output The output variant object
 */
void BuildPeerTracker(PeerTracker const &peer_tracker, variant::Variant &output)
{
  output                        = variant::Variant::Object();
  output["knownPeerCount"]      = peer_tracker.known_peer_count();
  output["activeBucketCount"]   = peer_tracker.active_buckets();
  output["firstNonEmptyBucket"] = peer_tracker.first_non_empty_bucket();

  BuildConnectionPriorities(peer_tracker, output["connectionPriority"]);
  BuildPeerSet(peer_tracker.keep_connections(), output["keepConnections"]);
  BuildPeerSet(peer_tracker.longrange_connections(), output["longrangeConnections"]);
  BuildPeerSet(peer_tracker.incoming(), output["incoming"]);
  BuildPeerSet(peer_tracker.outgoing(), output["outgoing"]);
  BuildPeerSet(peer_tracker.all_peers(), output["allPeers"]);
  BuildPeerSet(peer_tracker.desired_peers(), output["desiredPeers"]);
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

void BuildEchoCache(Router::EchoCache const &echo_cache, variant::Variant &output)
{
  output = variant::Variant::Array(echo_cache.size());

  std::size_t idx{0};
  for (auto const &element : echo_cache)
  {
    auto &entry = output[idx++] = variant::Variant::Object();

    entry["id"]        = element.first;
    entry["timestamp"] = element.second.time_since_epoch().count();
  }
}

/**
 * Build the JSON representation for the status of a given muddle
 *
 * @param muddle The muddle to evaluate
 * @param output The output variant object
 */
void BuildMuddleStatus(Muddle const &muddle, variant::Variant &output, bool extended)
{
  output                    = variant::Variant::Object();
  output["network"]         = muddle.GetNetwork().ToString();
  output["address"]         = muddle.GetAddress().ToBase64();
  output["externalAddress"] = muddle.GetExternalAddress();

  auto const &listening_ports = muddle.GetListeningPorts();
  auto &port_list = output["listeningPorts"] = variant::Variant::Array(listening_ports.size());
  for (std::size_t i = 0; i < listening_ports.size(); ++i)
  {
    port_list[i] = listening_ports.at(i);
  }

  BuildConnectionList(muddle.connection_register(), output["connections"]);

  BuildPeerLists(muddle.connection_list(), output["peers"]);
  // TODO(tfr): remove  BuildPeerSelection(muddle.peer_selector(), output["peerSelection"]);
  BuildPeerTracker(muddle.peer_tracker(), output["peerTracker"]);

  if (extended)
  {
    BuildEchoCache(muddle.router().echo_cache(), output["echoCache"]);
  }
}

/**
 * Filter a given muddle registry map for networks matching a specified
 *
 * @param map The map of muddle instances that need to be filtered
 * @param target_network The target network to be extracted
 * @return The filtered map of muddle instances
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
      BuildMuddleStatus(*muddle, output[index], false);
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
