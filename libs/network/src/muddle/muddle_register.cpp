#include "network/management/abstract_connection.hpp"
#include "network/muddle/muddle_register.hpp"
#include "network/muddle/dispatcher.hpp"

namespace fetch {
namespace muddle {

MuddleRegister::MuddleRegister(Dispatcher &dispatcher)
  : dispatcher_(dispatcher)
{
}

void MuddleRegister::VisitConnectionMap(MuddleRegister::ConnectionMapCallback const &cb)
{
  Lock lock(connection_map_lock_);
  cb(connection_map_);
}

MuddleRegister::ConnectionPtr MuddleRegister::LookupConnection(ConnectionHandle handle) const
{
  ConnectionPtr conn;

  {
    Lock lock(connection_map_lock_);

    auto it = connection_map_.find(handle);
    if (it != connection_map_.end())
    {
      conn = it->second;
    }
  }

  return conn;
}

void MuddleRegister::Enter(std::weak_ptr<network::AbstractConnection> const &ptr)
{
  Lock lock(connection_map_lock_);

  auto strong_conn = ptr.lock();

#ifndef NDEBUG
  // extra level of debug
  if (connection_map_.find(strong_conn->handle()) != connection_map_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Trying to update an existing connection ID");
    return;
  }
#endif // !NDEBUG

  FETCH_LOG_INFO(LOGGING_NAME, "### Connection ", strong_conn->handle(), " started type: ", strong_conn->Type());

  // update
  connection_map_[strong_conn->handle()] = ptr;
}

void MuddleRegister::Leave(connection_handle_type id)
{
  {
    Lock lock(connection_map_lock_);

    FETCH_LOG_INFO(LOGGING_NAME, "### Connection ", id, " ended");

    auto it = connection_map_.find(id);
    if (it != connection_map_.end())
    {
      connection_map_.erase(it);
    }
  }

  // inform the dispatcher that the connection has failed (this can clear up all of the pending promises)
  dispatcher_.NotifyConnectionFailure(id);
}

} // namespace p2p
} // namespace fetch