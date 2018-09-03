#ifndef FETCH_BOOTSTRAP_MONITOR_HPP
#define FETCH_BOOTSTRAP_MONITOR_HPP

#include "core/byte_array/byte_array.hpp"
#include "core/mutex.hpp"
#include "crypto/identity.hpp"
#include "http/json_client.hpp"
#include "network/fetch_asio.hpp"

#include "constellation.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace fetch {

class BootstrapMonitor
{
public:
  using PeerList = Constellation::PeerList;
  using Identity = crypto::Identity;

  BootstrapMonitor(Identity const &identity, uint16_t port, uint32_t network_id)
    : network_id_(network_id), port_(port), identity_(identity)
  {}

  bool Start(PeerList &peers);
  void Stop();

  std::string const &external_address() const { return external_address_; }

private:
  using IoService      = asio::io_service;
  using Resolver       = asio::ip::tcp::resolver;
  using Socket         = asio::ip::tcp::socket;
  using ThreadPtr      = std::unique_ptr<std::thread>;
  using Buffer         = byte_array::ByteArray;
  using Flag           = std::atomic<bool>;
  using Mutex          = mutex::Mutex;
  using LockGuard      = std::lock_guard<Mutex>;
  using ConstByteArray = byte_array::ConstByteArray;

  uint32_t const network_id_;
  uint16_t const port_;
  Identity const identity_;
  std::string    external_address_;

  Flag      running_{false};
  ThreadPtr monitor_thread_;

  Mutex     io_mutex_{__LINE__, __FILE__};
  IoService io_service_;
  Resolver  resolver_{io_service_};
  Socket    socket_{io_service_};
  Buffer    buffer_;

  bool UpdateExternalAddress();

  bool RequestPeerList(PeerList &peers);
  bool RegisterNode();
  bool NotifyNode();

  void ThreadEntryPoint();
};

}  // namespace fetch

#endif  // FETCH_BOOTSTRAP_MONITOR_HPP
