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
#include "core/serializers/main_serializer.hpp"
#include "logging/logging.hpp"
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
  using NetworkManagerType = NetworkManager;
  using HandleType         = uint64_t;
  using ImplementationType = TCPClientImplementation;
  using PointerType        = std::shared_ptr<ImplementationType>;

  explicit TCPClient(NetworkManagerType network_manager)
    : pointer_{std::make_shared<ImplementationType>(network_manager)}
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

  ~TCPClient() = default;

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

  void OnMessage(std::function<void(network::MessageBuffer const &msg)> const &f)
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

  void Send(MessageBuffer const &msg) noexcept
  {
    pointer_->Send(msg);
  }

  HandleType handle() const noexcept
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

  typename ImplementationType::WeakPointerType connection_pointer()
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
  PointerType pointer_;
};

}  // namespace network
}  // namespace fetch
