#ifndef FETCH_P2P_ROUTER_HPP
#define FETCH_P2P_ROUTER_HPP

#include "core/mutex.hpp"
#include "network/details/thread_pool.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/muddle/subscription_registrar.hpp"

#include <memory>
#include <chrono>

namespace fetch {
namespace muddle {

class Packet;

class Dispatcher;

class MuddleRegister;

class Router : public MuddleEndpoint
{
public:
  using Address       = Packet::Address;
  using PacketPtr     = std::shared_ptr<Packet>;
  using Payload       = Packet::Payload;
  using ConnectionPtr = std::weak_ptr<network::AbstractConnection>;
  using Handle        = network::AbstractConnection::connection_handle_type;
  using ThreadPool    = network::ThreadPool;

  static constexpr char const *LOGGING_NAME = "MuddleRoute";

  // Construction / Destruction
  explicit Router(Address const &address, MuddleRegister const &reg, Dispatcher &dispatcher);
  Router(Router const &) = delete;
  Router(Router &&) = delete;
  ~Router() override = default;

  // Start / Stop
  void Start();
  void Stop();

  // Operators
  Router &operator=(Router const &) = delete;
  Router &operator=(Router &&) = delete;

  void Route(Handle handle, PacketPtr packet);

  void AddConnection(Handle handle);
  void RemoveConnection(Handle handle);

  /// @name Endpoint Methods (Publicly visible)
  /// @{
  void Send(Address const &address,
            uint16_t service,
            uint16_t channel,
            Payload const &message) override;

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload) override;

  void Broadcast(uint16_t service, uint16_t channel, Payload const &payload) override;

  Response Exchange(Address const &address, uint16_t service, uint16_t channel,
                    Payload const &request) override;

  SubscriptionPtr Subscribe(uint16_t service, uint16_t channel) override;
  SubscriptionPtr Subscribe(Address const &address, uint16_t service, uint16_t channel) override;
  /// @}

private:

  struct RoutingData
  {
    bool   direct = false;
    Handle handle = 0;
  };

  using RoutingTable = std::unordered_map<Packet::RawAddress, RoutingData>;
  using HandleMap = std::unordered_map<Handle, std::unordered_set<Packet::RawAddress>>;
  using Mutex = mutex::Mutex;
  using Clock = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using EchoCache = std::unordered_map<uint64_t, Timepoint>;

  bool AssociateHandleWithAddress(Handle handle, Packet::RawAddress const &address, bool direct);

  Handle LookupHandle(Packet::RawAddress const &address) const;

  void SendToConnection(Handle handle, PacketPtr packet);
  void RoutePacket(PacketPtr packet, bool external = true);
  void DispatchDirect(Handle handle, PacketPtr packet);

  void DispatchPacket(PacketPtr packet);

  bool IsEcho(Packet const &packet, bool register_echo = true);

  Address const        address_;
  MuddleRegister const &register_;
  Dispatcher           &dispatcher_;
  SubscriptionRegistrar registrar_;

  mutable Mutex routing_table_lock_{__LINE__, __FILE__};
  RoutingTable  routing_table_;           ///< The map routing table from address to handle (Protected by routing_table_lock_)
  HandleMap     routing_table_handles_;   ///< The map of handles to address (Protected by routing_table_lock_)

  mutable Mutex echo_cache_lock_{__LINE__, __FILE__};
  EchoCache echo_cache_;

  ThreadPool            dispatch_thread_pool_;
};

} // namespace p2p
} // namespace fetch



#endif //FETCH_P2P_ROUTER_HPP
