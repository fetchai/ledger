#ifndef FETCH_P2P_MUDDLE_REGISTER_HPP
#define FETCH_P2P_MUDDLE_REGISTER_HPP

#include "core/mutex.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "network/management/abstract_connection_register.hpp"

#include <thread>
#include <functional>
#include <unordered_map>

namespace fetch {
namespace muddle {

class Dispatcher;

/**
 * The Muddle registers monitors all incoming and outgoing connections maintained in a given muddle.
 */
class MuddleRegister : public network::AbstractConnectionRegister
{
public:
  // TODO(EJF): Apply globally
  using ConnectionHandle = connection_handle_type;
  using ConnectionPtr = std::weak_ptr<network::AbstractConnection>;
  using ConnectionMap = std::unordered_map<ConnectionHandle, ConnectionPtr>;
  using ConnectionMapCallback = std::function<void(ConnectionMap const &)>;
  using ConstByteArray = byte_array::ConstByteArray;
  using Mutex = mutex::Mutex;

  static constexpr char const *LOGGING_NAME = "MuddleReg";

  // Construction / Destruction
  MuddleRegister(Dispatcher &dispatcher);
  MuddleRegister(MuddleRegister const &) = delete;
  MuddleRegister(MuddleRegister &&) = delete;
  ~MuddleRegister() = default;

  // Operators
  MuddleRegister &operator=(MuddleRegister const &) = delete;
  MuddleRegister &operator=(MuddleRegister &&) = delete;

  /// @name Callback
  /// @{
  void VisitConnectionMap(ConnectionMapCallback const &cb);
  /// @}

  void Broadcast(ConstByteArray const &data) const;
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
  Dispatcher   &dispatcher_;
};

} // namespace p2p
} // namespace fetch

#endif //FETCH_P2P_MUDDLE_REGISTER_HPP
