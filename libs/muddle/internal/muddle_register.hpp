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
#include "muddle/address.hpp"
#include "network/management/abstract_connection_register.hpp"
#include "telemetry/telemetry.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace network {
class AbstractConnection;
}  // namespace network
namespace muddle {

class Dispatcher;
class Router;
class NetworkId;

/**
 * The Muddle registers monitors all incoming and outgoing connections maintained in a given muddle.
 */
class MuddleRegister : public network::AbstractConnectionRegister
{
public:
  using ConnectionHandle       = ConnectionHandleType;
  using WeakConnectionPtr      = std::weak_ptr<network::AbstractConnection>;
  using ConnectionMap          = std::unordered_map<ConnectionHandle, WeakConnectionPtr>;
  using ConnectionMapCallback  = std::function<void(ConnectionMap const &)>;
  using ConstByteArray         = byte_array::ConstByteArray;
  using Handle                 = ConnectionHandleType;
  using ConnectionLeftCallback = std::function<void(Handle)>;

  enum class UpdateStatus
  {
    HANDLE_NOT_FOUND,
    NEW_ADDRESS,
    DUPLICATE_ADDRESS,
  };

  struct Entry
  {
    WeakConnectionPtr connection;
    Handle            handle{0};
    Address           address;
    bool              outgoing{false};

    explicit Entry(WeakConnectionPtr c);
    ~Entry() = default;
  };

  using EntryPtr     = std::shared_ptr<Entry>;
  using HandleIndex  = std::unordered_map<ConnectionHandle, EntryPtr>;
  using AddressIndex = std::unordered_multimap<Address, EntryPtr>;

  // Construction / Destruction
  explicit MuddleRegister(NetworkId const &network);
  MuddleRegister(MuddleRegister const &) = delete;
  MuddleRegister(MuddleRegister &&)      = delete;
  ~MuddleRegister() override             = default;

  // Operators
  MuddleRegister &operator=(MuddleRegister const &) = delete;
  MuddleRegister &operator=(MuddleRegister &&) = delete;

  void              OnConnectionLeft(ConnectionLeftCallback cb);
  void              Broadcast(ConstByteArray const &data) const;
  WeakConnectionPtr LookupConnection(ConnectionHandle handle) const;
  WeakConnectionPtr LookupConnection(Address const &address) const;

  bool         IsEmpty() const;
  UpdateStatus UpdateAddress(ConnectionHandle handle, Address const &address);
  bool         HasAddress(Address const &address) const;

  std::vector<Address>        GetCurrentConnectionAddresses() const;
  std::unordered_set<Address> GetCurrentAddressSet() const;
  std::unordered_set<Address> GetIncomingAddressSet() const;
  std::unordered_set<Address> GetOutgoingAddressSet() const;

  // Raw Access
  HandleIndex  GetHandleIndex() const;
  AddressIndex GetAddressIndex() const;

protected:
  /// @name Connection Event Handlers
  /// @{
  void Enter(WeakConnectionPtr const &ptr) override;
  void Leave(ConnectionHandleType handle) override;
  /// @}

private:
  std::string const name_;
  char const *const logging_name_{name_.c_str()};

  mutable Mutex          lock_;
  ConnectionLeftCallback left_callback_;
  HandleIndex            handle_index_;
  AddressIndex           address_index_;
};

}  // namespace muddle
}  // namespace fetch
