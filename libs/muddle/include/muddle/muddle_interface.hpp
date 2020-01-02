#pragma once
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

#include "moment/clock_interfaces.hpp"
#include "muddle/address.hpp"
#include "muddle/peer_selection_mode.hpp"
#include "muddle/tracker_configuration.hpp"
#include "network/uri.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace crypto {
class Prover;
class Identity;
}  // namespace crypto

namespace network {
class NetworkManager;
}

namespace muddle {

class MuddleEndpoint;
class NetworkId;

class MuddleInterface
{
public:
  enum class Confidence
  {
    DEFAULT,
    WHITELIST,
    BLACKLIST
  };

  using Peers          = std::unordered_set<std::string>;
  using Uris           = std::unordered_set<network::Uri>;
  using Ports          = std::vector<uint16_t>;
  using PortMapping    = std::unordered_map<uint16_t, uint16_t>;
  using Addresses      = std::unordered_set<Address>;
  using ConfidenceMap  = std::unordered_map<Address, Confidence>;
  using AddressHints   = std::unordered_map<Address, network::Uri>;
  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;
  using Duration       = ClockInterface::Duration;

  // Construction / Destruction
  MuddleInterface()          = default;
  virtual ~MuddleInterface() = default;

  constexpr static Duration NeverExpire()
  {
    return std::chrono::duration_cast<Duration>(std::chrono::hours(1024 * 24));
  }

  /// @name Muddle Setup
  /// @{

  virtual void SetPeerTableFile(std::string const &filename) = 0;

  /**
   * Start the muddle instance connecting to the initial set of peers and listing on the specified
   * set of ports
   *
   * @param peers The initial set of peers that muddle should connect to
   * @param ports The set of ports to listen on. Zero signals a random port
   * @return true if successful, otherwise false
   */
  virtual bool Start(Peers const &peers, Ports const &ports) = 0;

  /**
   * Start the muddle instance connecting to the initial set of peers and listing on the specified
   * set of ports
   *
   * @param peers The initial set of peers that muddle should connect to
   * @param ports The set of ports to listen on. Zero signals a random port
   * @return true if successful, otherwise false
   */
  virtual bool Start(Uris const &peers, Ports const &ports) = 0;

  /**
   * Start the muddle instance connecting to the initial set of peers and listing on the specified
   * set of ports
   *
   * @param peers The initial set of peers that muddle should connect to
   * @param port_mapping The maps
   * @return true if successful, otherwise false
   */
  virtual bool Start(Uris const &peers, PortMapping const &port_mapping) = 0;

  /**
   * Start the muddle instance listing on the specified set of ports
   *
   * @param ports The set of ports to listen on. Zero signals a random port
   * @return true if successful, otherwise false
   */
  virtual bool Start(Ports const &ports) = 0;

  /**
   * Stop the muddle instance, this will cause all the connected to close
   */
  virtual void Stop() = 0;

  /**
   * Get the endpoint interface for this muddle instance/
   *
   * @return The endpoint pointer
   */
  virtual MuddleEndpoint &GetEndpoint() = 0;

  /// @}

  /// @name Muddle Status
  /// @{

  /**
   * Get the associated network for this muddle instance
   *
   * @return The current network
   */
  virtual NetworkId const &GetNetwork() const = 0;

  /**
   * Get the address of this muddle node
   *
   * @return The address of the node
   */
  virtual Address const &GetAddress() const = 0;

  /**
   * Get the external address of the muddle
   *
   * @return The external address of the node
   */
  virtual std::string const &GetExternalAddress() const = 0;

  /**
   * Get the set of ports that the server is currently listening on
   *
   * @return The set of server ports
   */
  virtual Ports GetListeningPorts() const = 0;

  /**
   * Get the set of addresses to whom this node is directly connected to
   *
   * @return The set of addresses
   */
  virtual Addresses GetDirectlyConnectedPeers() const = 0;

  /**
   * Get the set of addresses of peers that are connected directly to this node
   *
   * @return The set of addresses
   */
  virtual Addresses GetIncomingConnectedPeers() const = 0;

  /**
   * Get the set of addresses of peers that we are directly connected to
   *
   * @return The set of peers
   */
  virtual Addresses GetOutgoingConnectedPeers() const = 0;

  /**
   * Get the number of peers that are directly connected to this node
   *
   * @return The number of directly connected peers
   */
  virtual std::size_t GetNumDirectlyConnectedPeers() const = 0;

  /**
   * Determine if we are directly connected to the specified address
   *
   * @param address The address to check
   * @return true if directly connected, otherwise false
   */
  virtual bool IsDirectlyConnected(Address const &address) const = 0;

  /**
   * Determines if the muddle is trying to establish/have established
   * a connection to an address.
   *
   * @param address The address to check
   * @return true if directly connected or connecting, otherwise false
   */
  virtual bool IsConnectingOrConnected(Address const &address) const = 0;

  /// @}

  /// @name Peer Control
  /// @{

  /**
   * Get the set of addresses that have been requested to connect to
   *
   * @return The set of addresses
   */
  virtual Addresses GetRequestedPeers() const = 0;

  /**
   * Request that muddle attempts to connect to the specified address
   *
   * @param address The requested address to connect to
   */
  virtual void ConnectTo(Address const &address, Duration const &expire = NeverExpire()) = 0;

  /**
   * Request that muddle attempts to connect to the specified set of addresses
   *
   * @param addresses The set of addresses
   */
  virtual void ConnectTo(Addresses const &addresses, Duration const &expire = NeverExpire()) = 0;

  /**
   * Request that muddle attempts to connect to the specified URI.
   *
   * @param uri The URI
   */
  virtual void ConnectTo(network::Uri const &uri, Duration const &expire = NeverExpire()) = 0;

  /**
   * Connect to a specified address with the provided URI hint
   *
   * @param address The address to connect to
   * @param uri_hint The hint to the connection URI
   */
  virtual void ConnectTo(Address const &address, network::Uri const &uri_hint,
                         Duration const &expire = NeverExpire()) = 0;

  /**
   * Connect to the specified addresses with the provided connection hints
   *
   * @param address_hints The map of address => URI hint
   */
  virtual void ConnectTo(AddressHints const &address_hints,
                         Duration const &    expire = NeverExpire()) = 0;

  /**
   * Request that muddle disconnected from the specified address
   *
   * @param address The requested address to disconnect from
   */
  virtual void DisconnectFrom(Address const &address) = 0;

  /**
   * Request that muddle disconnects from the specified set of addresses
   *
   * @param addresses The set of addresses
   */
  virtual void DisconnectFrom(Addresses const &addresses) = 0;

  /**
   * Update the confidence for a specified address to specified level
   *
   * @param address The address to be updated
   * @param confidence The confidence level to be set
   */
  virtual void SetConfidence(Address const &address, Confidence confidence) = 0;

  /**
   * Update the confidence for all the specified addresses with the specified level
   *
   * @param addresses The set of addresses to update
   * @param confidence The confidence level to be used
   */
  virtual void SetConfidence(Addresses const &addresses, Confidence confidence) = 0;

  /**
   * Update a map of address to confidence level
   *
   * @param map The map of address to confidence level
   */
  virtual void SetConfidence(ConfidenceMap const &map) = 0;

  /**
   * Sets the tracker configuration
   *
   * @param config The configuration for the peer tracker
   */
  virtual void SetTrackerConfiguration(TrackerConfiguration const &config) = 0;
  /// @}
};

using MuddlePtr = std::shared_ptr<MuddleInterface>;
using ProverPtr = std::shared_ptr<crypto::Prover>;

// creation
MuddlePtr CreateMuddle(NetworkId const &network, ProverPtr certificate,
                       network::NetworkManager const &nm, std::string const &external_address);
MuddlePtr CreateMuddle(char const network[4], ProverPtr certificate,
                       network::NetworkManager const &nm, std::string const &external_address);
MuddlePtr CreateMuddle(NetworkId const &network, network::NetworkManager const &nm,
                       std::string const &external_address);
MuddlePtr CreateMuddle(char const network[4], network::NetworkManager const &nm,
                       std::string const &external_address);

}  // namespace muddle
}  // namespace fetch
