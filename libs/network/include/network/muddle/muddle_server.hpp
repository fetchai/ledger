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

#include "core/logger.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "network/muddle/router.hpp"
#include "network/tcp/abstract_server.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace fetch {
namespace muddle {

/**
 * The muddle server is just an simple specialisation of a network server. Its job is to marshall
 * the incoming data into a muddle packet which can then can be routed in the system.
 *
 * @tparam T A network server class, for example TCPServer. This class must be derived from
 * AbstractNetworkServer.
 */
template <typename NETWORK_SERVER>
class MuddleServer final : public NETWORK_SERVER
{
public:
  using connection_handle_type = network::AbstractNetworkServer::connection_handle_type;
  using message_type           = network::message_type;
  using ByteArrayBuffer        = serializers::ByteArrayBuffer;

  // ensure the NETWORK_SERVER type that we are using is actually what we where expecting
  static_assert(std::is_base_of<network::AbstractNetworkServer, NETWORK_SERVER>::value,
                "The network server type must be of network::AbstractNetworkServer");

  static constexpr char const *LOGGING_NAME = "MuddleSrv";

  /**
   * Constructs the instance of this server
   *
   * @tparam Args The types associated with the constructor for the NETWORK_SERVER type
   *
   * @param router The reference to the router
   * @param args The arguments for the underlying network server
   */
  template <typename... Args>
  MuddleServer(Router &router, Args... args)
    : NETWORK_SERVER(args...)
    , router_(router)
  {}
  MuddleServer(MuddleServer const &) = delete;
  MuddleServer(MuddleServer &&)      = delete;
  ~MuddleServer()                    = default;

  // Operators
  MuddleServer &operator=(MuddleServer const &) = delete;
  MuddleServer &operator=(MuddleServer &&) = delete;

private:
  /**
   * Handles incoming request from the underlying network server.
   *
   * The job of this function is to un-marshall these incoming packets and then dispatch them to the
   * router.
   *
   * @param client The handle to connection which generated this message
   * @param msg The payload of the message
   */
  void PushRequest(connection_handle_type client, message_type const &msg) override
  {
    try
    {
      LOG_STACK_TRACE_POINT;
      // un-marshall the data
      ByteArrayBuffer buffer(msg);

      auto packet = std::make_shared<Packet>();

      {
        buffer >> *packet;
      }

      // dispatch the message to router
      router_.Route(client, packet);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "port = ", this->port());
      FETCH_LOG_WARN(LOGGING_NAME, "byte array size = ", msg.size());
      FETCH_LOG_WARN(LOGGING_NAME, "byte array = ", byte_array::ToHex(msg));
      FETCH_LOG_ERROR(LOGGING_NAME, "Error processing packet from ", client, " error: ", ex.what());
      throw;
    }
  }

  Router &router_;  ///< The reference to the router to be used to dispatch the incoming requests
};

}  // namespace muddle
}  // namespace fetch
