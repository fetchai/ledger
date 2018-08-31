#ifndef FETCH_P2P_MUDDLE_REGISTER_HPP
#define FETCH_P2P_MUDDLE_REGISTER_HPP

#include "core/mutex.hpp"
#include "network/management/abstract_connection_register.hpp"

#include <thread>
#include <functional>
#include <unordered_map>

namespace fetch {
namespace muddle {

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

  static constexpr char const *LOGGING_NAME = "MuddleReg";

  // Construction / Destruction
  MuddleRegister() = default;
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

  ConnectionPtr LookupConnection(ConnectionHandle handle) const;

protected:

  /// @name Connection Event Handlers
  /// @{
  void Enter(std::weak_ptr<network::AbstractConnection> const &ptr) override;
  void Leave(connection_handle_type id) override;

  void SignalRead(connection_handle_type id) override;
  void SignalSend(connection_handle_type id) override;
  /// @}

private:

  using Mutex = mutex::Mutex;
  using Lock = std::lock_guard<Mutex>;

  mutable Mutex connection_map_lock_{__LINE__, __FILE__};
  ConnectionMap connection_map_;

};

} // namespace p2p
} // namespace fetch

#endif //FETCH_P2P_MUDDLE_REGISTER_HPP
