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

#include "network/muddle/muddle_register.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/muddle/dispatcher.hpp"

namespace fetch {
namespace muddle {

/**
 * Construct the connection register
 *
 * @param dispatcher The reference to the dispatcher
 */
MuddleRegister::MuddleRegister(Dispatcher &dispatcher)
  : dispatcher_(dispatcher)
{}

/**
 * Execute a specified callback over all elements of the connection map
 *
 * @param cb The specified callback to execute
 */
void MuddleRegister::VisitConnectionMap(MuddleRegister::ConnectionMapCallback const &cb)
{
  LOG_STACK_TRACE_POINT;
  FETCH_LOCK(connection_map_lock_);
  cb(connection_map_);
}

/**
 * Broadcast data to all active connections
 *
 * @param data The data to be broadcast
 */
void MuddleRegister::Broadcast(ConstByteArray const &data) const
{
  LOG_STACK_TRACE_POINT;  
  FETCH_LOCK(connection_map_lock_);
  FETCH_LOG_DEBUG(LOGGING_NAME, "Broadcasting message.");

  // loop through all of our current connections
  for (auto const &elem : connection_map_)
  {
    // ensure the connection is valid
    auto connection = elem.second.lock();
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
MuddleRegister::ConnectionPtr MuddleRegister::LookupConnection(ConnectionHandle handle) const
{
  LOG_STACK_TRACE_POINT;  
  ConnectionPtr conn;

  {
    FETCH_LOCK(connection_map_lock_);

    auto it = connection_map_.find(handle);
    if (it != connection_map_.end())
    {
      conn = it->second;
    }
  }

  return conn;
}

/**
 * Callback triggered when a new connection is established
 *
 * @param ptr The new connection pointer
 */
void MuddleRegister::Enter(ConnectionPtr const &ptr)
{
  LOG_STACK_TRACE_POINT;
  FETCH_LOCK(connection_map_lock_);

  auto strong_conn = ptr.lock();

#ifndef NDEBUG
  // extra level of debug
  if (connection_map_.find(strong_conn->handle()) != connection_map_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Trying to update an existing connection ID");
    return;
  }
#endif  // !NDEBUG

  FETCH_LOG_DEBUG(LOGGING_NAME, "### Connection ", strong_conn->handle(),
                  " started type: ", strong_conn->Type());

  // update
  connection_map_[strong_conn->handle()] = ptr;
}

/**
 * Callback triggered when a connection is destroyed
 *
 * @param id The handle of the dying connection
 */
void MuddleRegister::Leave(connection_handle_type id)
{
  LOG_STACK_TRACE_POINT;
  {
    FETCH_LOCK(connection_map_lock_);

    FETCH_LOG_DEBUG(LOGGING_NAME, "### Connection ", id, " ended");

    auto it = connection_map_.find(id);
    if (it != connection_map_.end())
    {
      connection_map_.erase(it);
    }
  }

  // inform the dispatcher that the connection has failed (this can clear up all of the pending
  // promises)
  dispatcher_.NotifyConnectionFailure(id);
}

}  // namespace muddle
}  // namespace fetch
