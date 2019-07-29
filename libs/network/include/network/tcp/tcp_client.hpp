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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/main_serializer.hpp"
#include "network/management/network_manager.hpp"
#include "network/message.hpp"
#include "network/tcp/client_implementation.hpp"

#include "network/fetch_asio.hpp"

#include <atomic>
#include <memory>

namespace fetch {
namespace network {

class TCPClient
{
public:
  using network_manager_type = NetworkManager;
  using handle_type          = uint64_t;
  using implementation_type  = TCPClientImplementation;
  using pointer_type         = std::shared_ptr<implementation_type>;

  explicit TCPClient(network_manager_type network_manager)
    : pointer_{std::make_shared<implementation_type>(network_manager)}
  {
    // Note we register handles here, but do not connect until the base class
    // constructed
  }

  // Disable copy and move to avoid races when creating a closure
  // as inherited classes still won't be constructed at this point
  TCPClient(TCPClient const &rhs) = delete;
  TCPClient(TCPClient &&rhs)      = delete;
  TCPClient &operator=(TCPClient const &rhs) = delete;
  TCPClient &operator=(TCPClient &&rhs) = delete;

  ~TCPClient() noexcept
  {
    LOG_STACK_TRACE_POINT;
  }

  void Connect(byte_array::ConstByteArray const &host, uint16_t port)
  {
    pointer_->Connect(host, port);
  }

  void Connect(byte_array::ConstByteArray const &host, byte_array::ConstByteArray const &port)
  {
    pointer_->Connect(host, port);
  }

  // For safety, this MUST be called by the base class in its destructor
  // As closures to that class exist in the client implementation
  void Cleanup() noexcept
  {
    if (pointer_)
    {
      pointer_->ClearClosures();
      pointer_->Close();
    }
  }

  void OnMessage(std::function<void(network::message_type const &msg)> const &f)
  {
    if (pointer_)
    {
      pointer_->OnMessage(f);
    }
  }

  void OnConnectionFailed(std::function<void()> const &fnc)
  {
    if (pointer_)
    {
      pointer_->OnConnectionFailed(fnc);
    }
  }

  void Close() const noexcept
  {
    pointer_->Close();
  }

  bool Closed() const noexcept
  {
    return pointer_->Closed();
  }

  void Send(message_type const &msg) noexcept
  {
    pointer_->Send(msg);
  }

  handle_type handle() const noexcept
  {
    return pointer_->handle();
  }

  std::string Address() const noexcept
  {
    return pointer_->Address();
  }

  bool is_alive() const noexcept
  {
    return pointer_->is_alive();
  }

  typename implementation_type::weak_ptr_type connection_pointer()
  {
    return pointer_->connection_pointer();
  }

  // Blocking function to wait until connection is alive
  bool WaitForAlive(std::size_t milliseconds) const
  {
    for (std::size_t i = 0; i < milliseconds; i += 10)
    {
      if (pointer_->is_alive())
      {
        return true;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return false;
  }

protected:
  pointer_type pointer_;
};

}  // namespace network
}  // namespace fetch
