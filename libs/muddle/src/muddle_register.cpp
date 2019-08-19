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

#include "dispatcher.hpp"
#include "muddle_register.hpp"

#include "network/management/abstract_connection.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

namespace fetch {
namespace muddle {

using telemetry::Registry;

MuddleRegister::Entry::Entry(WeakConnectionPtr c)
  : connection{std::move(c)}
{
  auto conn = connection.lock();
  if (conn)
  {
    handle   = conn->handle();
    outgoing = conn->Type() == network::AbstractConnection::TYPE_OUTGOING;
  }
}

/**
 * Broadcast data to all active connections
 *
 * @param data The data to be broadcast
 */
void MuddleRegister::Broadcast(ConstByteArray const &data) const
{
  FETCH_LOCK(lock_);

  // loop through all of our current connections
  for (auto const &elem : handle_index_)
  {
    // ensure the connection is valid
    auto connection = elem.second->connection.lock();
    if (connection)
    {
      // schedule sending of the data
      connection->Send(data);
    }
  }
}

/**
 * Lookup a connection given a specified handle
 *
 * @param handle The handle of the requested connection
 * @return A valid connection if successful, otherwise an invalid one
 */
MuddleRegister::WeakConnectionPtr MuddleRegister::LookupConnection(ConnectionHandle handle) const
{
  WeakConnectionPtr conn{};

  {
    FETCH_LOCK(lock_);

    auto it = handle_index_.find(handle);
    if (it != handle_index_.end())
    {
      conn = it->second->connection;
    }
  }

  return conn;
}

MuddleRegister::WeakConnectionPtr MuddleRegister::LookupConnection(Address const &address) const
{
  WeakConnectionPtr conn{};

  {
    FETCH_LOCK(lock_);

    auto it = address_index_.find(address);
    if (it != address_index_.end())
    {
      conn = it->second->connection;
    }
  }

  return conn;
}

void MuddleRegister::DisconnectAll()
{
  FETCH_LOCK(lock_);

  for (auto const &element : handle_index_)
  {
    auto conn = element.second->connection.lock();
    if (conn)
    {
      conn->Close();
    }
  }

  // tear it all down
  handle_index_.clear();
  address_index_.clear();
}

bool MuddleRegister::IsEmpty() const
{
  FETCH_LOCK(lock_);

#ifndef NDEBUG
  if (handle_index_.empty())
  {
    if (!address_index_.empty())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Logical inconsistency");
      assert(false);
    }
  }
#endif // !NDEBUG

  return handle_index_.empty();
}

MuddleRegister::UpdateStatus MuddleRegister::UpdateAddress(ConnectionHandle handle, Address const &address)
{
  UpdateStatus status{UpdateStatus::HANDLE_NOT_FOUND};

  FETCH_LOCK(lock_);

  auto it = handle_index_.find(handle);
  if (it != handle_index_.end())
  {
    // capture the entry and update the internal field
    auto entry = it->second;
    entry->address = address.Copy();

    // determine if this a duplicate address
    bool const duplicate_address = address_index_.find(address) != address_index_.end();

    bool const duplicate_entry =
        std::find_if(address_index_.begin(), address_index_.end(),
                     [handle](AddressIndex::value_type const &entry) {
                       return entry.second && entry.second->handle == handle;
                     }) != address_index_.end();

    // only add the entry to the map if there isn't already one
    if (!duplicate_entry)
    {
      address_index_.emplace(entry->address, entry);
    }

    // update the status based on if the address is a duplicate
    status = (duplicate_address) ? UpdateStatus::DUPLICATE_ADDRESS : UpdateStatus::NEW_ADDRESS;
  }

  return status;
}

bool MuddleRegister::HasAddress(Address const &address) const
{
  FETCH_LOCK(lock_);
  return address_index_.find(address) != address_index_.end();
}

std::vector<Address> MuddleRegister::GetCurrentConnectionAddresses() const
{
  std::vector<Address> addresses{};

  FETCH_LOCK(lock_);
  addresses.reserve(address_index_.size());
  for (auto const &element : address_index_)
  {
    addresses.emplace_back(element.first);
  }

  return addresses;
}

std::unordered_set<Address> MuddleRegister::GetCurrentAddressSet() const
{
  std::unordered_set<Address> addresses{};

  FETCH_LOCK(lock_);
  addresses.reserve(address_index_.size());
  for (auto const &element : address_index_)
  {
    addresses.emplace(element.first);
  }

  return addresses;
}

std::unordered_set<Address> MuddleRegister::GetIncomingAddressSet() const
{
  std::unordered_set<Address> addresses{};

  FETCH_LOCK(lock_);
  addresses.reserve(address_index_.size());
  for (auto const &element : address_index_)
  {
    if (!element.second->outgoing)
    {
      addresses.emplace(element.first);
    }
  }

  return addresses;

}

std::unordered_set<Address> MuddleRegister::GetOutgoingAddressSet() const
{
  std::unordered_set<Address> addresses{};

  FETCH_LOCK(lock_);
  addresses.reserve(address_index_.size());
  for (auto const &element : address_index_)
  {
    if (element.second->outgoing)
    {
      addresses.emplace(element.first);
    }
  }

  return addresses;
}


MuddleRegister::HandleIndex MuddleRegister::GetHandleIndex() const
{
  FETCH_LOCK(lock_);
  return handle_index_;
}

MuddleRegister::AddressIndex MuddleRegister::GetAddressIndex() const
{
  FETCH_LOCK(lock_);
  return address_index_;
}

/**
 * Callback triggered when a new connection is established
 *
 * @param ptr The new connection pointer
 */
void MuddleRegister::Enter(WeakConnectionPtr const &ptr)
{
  FETCH_LOCK(lock_);

  auto strong_conn = ptr.lock();
  if (!strong_conn)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Attempting to register lost connection!");
    return;
  }

  // cache the handle
  auto const handle = strong_conn->handle();

  // extra level of debug
  if (handle_index_.find(handle) != handle_index_.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Trying to update an existing connection ID");
    return;
  }

  FETCH_LOG_TRACE(LOGGING_NAME, "### Connection ", handle, " started type: ", strong_conn->Type());

  // add the connection to the map
  handle_index_.emplace(handle, std::make_shared<Entry>(ptr));
}

/**
 * Callback triggered when a connection is destroyed
 *
 * @param id The handle of the dying connection
 */
void MuddleRegister::Leave(ConnectionHandle handle)
{
  FETCH_LOCK(lock_);

  FETCH_LOG_TRACE(LOGGING_NAME, "### Connection ", handle, " ended");

  auto it = handle_index_.find(handle);
  if (it != handle_index_.end())
  {
    std::size_t removal_count{0};
    for (;;)
    {
      // attempt to find a corresponding index in the addres map
      auto addr_it = std::find_if(address_index_.begin(), address_index_.end(),
                                  [&](AddressIndex::value_type const &entry) {
                                    return (!entry.second) || (entry.second->handle == handle);
                                  });

      if (addr_it == address_index_.end())
      {
        break;
      }

      // remove the entry
      address_index_.erase(addr_it);
      ++removal_count;
    }

    FETCH_LOG_WARN(LOGGING_NAME, "Removing ", removal_count, " address entries for handle: ", handle);

    handle_index_.erase(it);
  }
}

}  // namespace muddle
}  // namespace fetch
