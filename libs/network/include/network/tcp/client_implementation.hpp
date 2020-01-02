#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/mutex.hpp"
#include "core/serializers/main_serializer.hpp"
#include "logging/logging.hpp"
#include "network/fetch_asio.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/management/network_manager.hpp"
#include "network/message.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>

namespace fetch {
namespace network {

class TCPClientImplementation final : public AbstractConnection
{
public:
  using NetworkManagerType = NetworkManager;
  using SelfType           = std::weak_ptr<AbstractConnection>;
  using SharedSelfType     = std::shared_ptr<AbstractConnection>;
  using SocketType         = asio::ip::tcp::tcp::socket;
  using StrandType         = asio::io_service::strand;
  using ResolverType       = asio::ip::tcp::resolver;
  using MutexType          = std::mutex;

  static const uint64_t        NETWORK_MAGIC = 0xFE7C80A1FE7C80A1;
  static constexpr char const *LOGGING_NAME  = "TCPClientImpl";

  explicit TCPClientImplementation(NetworkManagerType const &network_manager) noexcept;
  TCPClientImplementation(TCPClientImplementation const &rhs) = delete;
  TCPClientImplementation(TCPClientImplementation &&rhs)      = delete;
  ~TCPClientImplementation() override;

  void Connect(byte_array::ConstByteArray const &host, uint16_t port);

  void Connect(byte_array::ConstByteArray const &host, byte_array::ConstByteArray const &port);

  bool is_alive() const override;

  void Send(MessageBuffer const &omsg, Callback const &success = nullptr,
            Callback const &fail = nullptr) override;

  uint16_t Type() const override;

  void Close() override;

  bool Closed() const override;

  TCPClientImplementation &operator=(TCPClientImplementation const &rhs) = delete;
  TCPClientImplementation &operator=(TCPClientImplementation &&rhs) = delete;

  static void SetHeader(byte_array::ByteArray &header, uint64_t bufSize);

private:
  NetworkManagerType networkManager_;
  // IO objects should be guaranteed to have lifetime less than the
  // io_service/networkManager
  std::weak_ptr<SocketType> socket_;
  std::weak_ptr<StrandType> strand_;

  MessageQueueType  write_queue_;
  mutable MutexType queue_mutex_;
  mutable MutexType io_creation_mutex_;

  mutable MutexType can_write_mutex_;
  bool              can_write_{true};
  bool              posted_close_ = false;

  mutable MutexType callback_mutex_;
  std::atomic<bool> connected_{false};

  void ReadHeader() noexcept;
  void ReadBody(byte_array::ByteArray const &header) noexcept;

  // Always executed in a run(), in a strand
  void WriteNext(SharedSelfType const &selfLock);
};

}  // namespace network
}  // namespace fetch
