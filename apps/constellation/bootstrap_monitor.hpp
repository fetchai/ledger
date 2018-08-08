#ifndef FETCH_BOOTSTRAP_MONITOR_HPP
#define FETCH_BOOTSTRAP_MONITOR_HPP

#include "core/script/variant.hpp"
#include "core/byte_array/byte_array.hpp"
#include "network/fetch_asio.hpp"
#include "constellation.hpp"

#include <thread>
#include <memory>
#include <atomic>

namespace fetch {

class BootstrapMonitor
{
public:
  using PeerList = Constellation::peer_list_type;

  BootstrapMonitor(uint16_t port, uint32_t network_id)
    : network_id_(network_id)
    , port_(port)
  {
  }

  bool Start(PeerList &peers);
  void Stop();

private:

  using IoService = asio::io_service;
  using Resolver = asio::ip::tcp::resolver;
  using Socket = asio::ip::tcp::socket;
  using ThreadPtr = std::unique_ptr<std::thread>;
  using Buffer = byte_array::ByteArray;
  using Flag = std::atomic<bool>;

  uint32_t const network_id_;
  uint16_t const port_;

  Flag running_{false};
  ThreadPtr monitor_thread_;

  IoService io_service_;
  Resolver resolver_{io_service_};
  Socket socket_{io_service_};
  Buffer buffer_;

  bool RequestPeerList(PeerList &peers);
  bool MakeRequest(script::Variant const &request, script::Variant &response);

  void ThreadEntryPoint();
};

} // namespace fetch

#endif //FETCH_BOOTSTRAP_MONITOR_HPP
