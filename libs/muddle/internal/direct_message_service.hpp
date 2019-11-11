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

#include "core/mutex.hpp"
#include "crypto/fnv.hpp"
#include "muddle/address.hpp"
#include "network/management/abstract_connection.hpp"

#include <string>

namespace fetch {
namespace muddle {

class Packet;
class Router;
class MuddleRegister;
class PeerConnectionList;
struct RoutingMessage;

class DirectMessageService
{
public:
  using Handle    = network::AbstractConnection::ConnectionHandleType;
  using PacketPtr = std::shared_ptr<Packet>;

  // Construction / Destruction
  DirectMessageService(Address address, Router &router, MuddleRegister &reg,
                       PeerConnectionList &peers);
  DirectMessageService(DirectMessageService const &) = delete;
  DirectMessageService(DirectMessageService &&)      = delete;
  ~DirectMessageService()                            = default;

  void InitiateConnection(Handle handle);
  void RequestDisconnect(Handle handle);
  void SignalConnectionLeft(Handle handle);

  // Operators
  DirectMessageService &operator=(DirectMessageService const &) = delete;
  DirectMessageService &operator=(DirectMessageService &&) = delete;

private:
  struct ConnectionData
  {
    Address address;
  };

  enum class Phase : uint8_t
  {
    INITIAL = 0,
    PREPARE,
    ACCEPT,
    ACKNOWLEDGED,
    ESTABLISHED,
  };

  struct Reservation
  {
    Handle handle{0};
    Phase  phase{Phase::INITIAL};
  };

  using Reservations = std::unordered_map<Address, Handle>;

  template <typename T>
  void SendMessageToConnection(Handle handle, T const &msg, bool exchange = false);

  void OnDirectMessage(Handle handle, PacketPtr const &packet);
  void OnRoutingMessage(Handle handle, PacketPtr const &packet, RoutingMessage const &msg);
  void OnRoutingPing(Handle handle, PacketPtr const &packet, RoutingMessage const &msg);
  void OnRoutingPong(Handle handle, PacketPtr const &packet, RoutingMessage const &msg);
  void OnRoutingRequest(Handle handle, PacketPtr const &packet, RoutingMessage const &msg);
  void OnRoutingAccepted(Handle handle, PacketPtr const &packet, RoutingMessage const &msg);
  void OnRoutingDisconnectRequest(Handle handle, PacketPtr const &packet,
                                  RoutingMessage const &msg);

  enum class UpdateStatus
  {
    NO_CHANGE,
    ADDED,
    REPLACED,
    DUPLICATE
  };

  static char const *ToString(UpdateStatus status);

  UpdateStatus UpdateReservation(Address const &address, Handle handle,
                                 Handle *previous_handle = nullptr);

  Address const       address_;
  std::string const   name_;
  char const *const   logging_name_{name_.c_str()};
  Router &            router_;
  MuddleRegister &    register_;
  PeerConnectionList &peers_;

  Mutex        lock_;
  Reservations reservations_;
};

}  // namespace muddle
}  // namespace fetch
