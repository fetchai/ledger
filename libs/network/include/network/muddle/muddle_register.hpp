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
#include "core/mutex.hpp"
#include "network/management/abstract_connection_register.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

namespace fetch {
namespace network {
class AbstractConnection;
}  // namespace network
namespace muddle {

class Dispatcher;

/**
 * The Muddle registers monitors all incoming and outgoing connections maintained in a given muddle.
 */
class MuddleRegister : public network::AbstractConnectionRegister
{
public:
  using ConnectionHandle      = connection_handle_type;
  using ConnectionPtr         = std::weak_ptr<network::AbstractConnection>;
  using ConnectionMap         = std::unordered_map<ConnectionHandle, ConnectionPtr>;
  using ConnectionMapCallback = std::function<void(ConnectionMap const &)>;
  using ConstByteArray        = byte_array::ConstByteArray;
  using Mutex                 = mutex::Mutex;

  static constexpr char const *LOGGING_NAME = "MuddleReg";

  // Construction / Destruction
  explicit MuddleRegister(Dispatcher &dispatcher);
  MuddleRegister(MuddleRegister const &) = delete;
  MuddleRegister(MuddleRegister &&)      = delete;
  ~MuddleRegister() override             = default;

  // Operators
  MuddleRegister &operator=(MuddleRegister const &) = delete;
  MuddleRegister &operator=(MuddleRegister &&) = delete;

  /// @name Callback
  /// @{
  void VisitConnectionMap(ConnectionMapCallback const &cb);
  /// @}

  void          Broadcast(ConstByteArray const &data) const;
  ConnectionPtr LookupConnection(ConnectionHandle handle) const;

protected:
  /// @name Connection Event Handlers
  /// @{
  void Enter(ConnectionPtr const &ptr) override;
  void Leave(connection_handle_type id) override;
  /// @}

private:
  mutable Mutex connection_map_lock_{__LINE__, __FILE__};
  ConnectionMap connection_map_;
  Dispatcher &  dispatcher_;
};

}  // namespace muddle
}  // namespace fetch
