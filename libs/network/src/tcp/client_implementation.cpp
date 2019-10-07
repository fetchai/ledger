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

#include "network/tcp/client_implementation.hpp"

namespace fetch {
namespace network {

TCPClientImplementation::TCPClientImplementation(NetworkManagerType const &network_manager) noexcept
  : networkManager_(network_manager)
{}

TCPClientImplementation::~TCPClientImplementation()
{
  if (!Closed() && !posted_close_)
  {
    Close();
  }
}

void TCPClientImplementation::Connect(byte_array::ConstByteArray const &host, uint16_t port)
{
  Connect(host, byte_array::ConstByteArray(std::to_string(port)));
}

void TCPClientImplementation::Connect(byte_array::ConstByteArray const &host,
                                      byte_array::ConstByteArray const &port)
{
  SelfType self = shared_from_this();

  FETCH_LOG_DEBUG(LOGGING_NAME, "Client posting connect");

  networkManager_.Post([this, self, host, port] {
    SharedSelfType selfLock = self.lock();
    if (!selfLock)
    {
      return;
    }

    // We get IO objects from the network manager, they will only be strong
    // while in the post
    auto strand = networkManager_.CreateIO<StrandType>();
    if (!strand)
    {
      return;
    }
    {
      FETCH_LOCK(io_creation_mutex_);
      strand_ = strand;
    }

    strand->post([this, self, host, port, strand] {
      SharedSelfType selfLock = self.lock();
      if (!selfLock)
      {
        return;
      }

      std::shared_ptr<SocketType> socket = networkManager_.CreateIO<SocketType>();

      {
        FETCH_LOCK(io_creation_mutex_);
        if (!posted_close_)
        {
          socket_ = socket;
        }
      }

      std::shared_ptr<ResolverType> res = networkManager_.CreateIO<ResolverType>();

      auto cb = [this, self, res, socket, strand, port](std::error_code ec,
                                                        ResolverType::iterator) {
        SharedSelfType selfLock = self.lock();
        if (!selfLock)
        {
          return;
        }

        FETCH_LOG_DEBUG(LOGGING_NAME, "Finished connecting.");
        if (!ec)
        {
          FETCH_LOG_DEBUG(LOGGING_NAME, "Connection established!");

          // Prevent this from throwing
          std::error_code         ec2;
          asio::ip::tcp::endpoint endpoint = (*socket).remote_endpoint(ec2);

          if (!ec2)
          {
            this->SetAddress(endpoint.address().to_string());
            this->SetPort(uint16_t(port.AsInt()));
            ReadHeader();
          }
          else
          {
            FETCH_LOG_ERROR(LOGGING_NAME,
                            "Failed to get endpoint of socket after connection: ", ec2.message());
          }
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Client failed to connect on port ", port, ": ",
                         ec.message());
          SignalLeave();
        }
      };

      if (socket && res)
      {
        std::error_code resolve_ec{};

        ResolverType::iterator it(res->resolve({std::string(host), std::string(port)}, resolve_ec));

        if (!resolve_ec)
        {
          assert(strand->running_in_this_thread());
          asio::async_connect(*socket, it, strand->wrap(cb));
        }
        else
        {
          FETCH_LOG_ERROR(LOGGING_NAME, "Resolution failure: ", resolve_ec.message());
          SignalLeave();
        }
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Failed to create valid socket");
        SignalLeave();
      }
    });  // end strand post
  });    // end NM post
}

bool TCPClientImplementation::is_alive() const
{
  FETCH_LOCK(io_creation_mutex_);
  return !socket_.expired() && connected_;
}

void TCPClientImplementation::Send(MessageType const &omsg)
{
  MessageType msg = omsg.Copy();
  if (!connected_)
  {
    return;
  }

  {
    FETCH_LOCK(queue_mutex_);
    write_queue_.push_back(msg);
  }

  SelfType                  self   = shared_from_this();
  std::weak_ptr<StrandType> strand = strand_;

  networkManager_.Post([this, self, strand] {
    SharedSelfType selfLock   = self.lock();
    auto           strandLock = strand_.lock();
    if (!selfLock || !strandLock)
    {
      return;
    }

    strandLock->post([this, selfLock] { WriteNext(selfLock); });
  });
}

uint16_t TCPClientImplementation::Type() const
{
  return AbstractConnection::TYPE_OUTGOING;
}

void TCPClientImplementation::Close()
{
  FETCH_LOCK(io_creation_mutex_);
  posted_close_                        = true;
  std::weak_ptr<SocketType> socketWeak = socket_;
  std::weak_ptr<StrandType> strandWeak = strand_;

  networkManager_.Post([socketWeak, strandWeak] {
    auto socket = socketWeak.lock();
    auto strand = strandWeak.lock();

    if (socket && strand)
    {
      strand->post([socket] {
        std::error_code dummy;
        socket->shutdown(asio::ip::tcp::socket::shutdown_both, dummy);
        socket->close(dummy);
      });
    }
  });
}

bool TCPClientImplementation::Closed() const
{
  return socket_.expired();
}

void TCPClientImplementation::ReadHeader() noexcept
{
  auto strand = strand_.lock();
  if (!strand)
  {
    return;
  }
  assert(strand->running_in_this_thread());

  SelfType              self   = shared_from_this();
  auto                  socket = socket_.lock();
  byte_array::ByteArray header;
  header.Resize(2 * sizeof(uint64_t));

  auto cb = [this, self, socket, header, strand](std::error_code ec, std::size_t) {
    SharedSelfType selfLock = self.lock();
    if (!selfLock)
    {
      return;
    }

    if (!ec)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Read message header.");
      ReadBody(header);
    }
    else if (!posted_close_)
    {
      // We expect to get an ec here when the socked is closed via a post
      FETCH_LOG_INFO(LOGGING_NAME, "Socket closed inside ReadHeader: ", ec.message());
      SignalLeave();
    }
  };

  if (socket)
  {
    assert(strand->running_in_this_thread());
    asio::async_read(*socket, asio::buffer(header.pointer(), header.size()), strand->wrap(cb));

    bool const previously_connected = connected_.exchange(true);

    if (!previously_connected)
    {
      SignalConnectionSuccess();
    }
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Socket no longer valid in ReadHeader");
    connected_ = false;
    SignalLeave();
  }
}

void TCPClientImplementation::ReadBody(byte_array::ByteArray const &header) noexcept
{
  auto strand = strand_.lock();
  assert(strand->running_in_this_thread());

  assert(header.size() >= sizeof(networkMagic_));
  uint64_t magic = *reinterpret_cast<const uint64_t *>(header.pointer());
  uint64_t size  = *reinterpret_cast<const uint64_t *>(header.pointer() + sizeof(uint64_t));

  if (magic != networkMagic_)
  {
    byte_array::ByteArray dummy;
    SetHeader(dummy, 0);
    dummy.Resize(16);

    FETCH_LOG_ERROR(LOGGING_NAME, "Magic incorrect during network read:\ngot:      ", ToHex(header),
                    "\nExpected: ", ToHex(byte_array::ByteArray(dummy)));
    return;
  }

  byte_array::ByteArray message;
  message.Resize(size);

  SelfType self   = shared_from_this();
  auto     socket = socket_.lock();
  auto     cb     = [this, self, message, socket, strand](std::error_code ec, std::size_t len) {
    FETCH_UNUSED(len);

    SharedSelfType selfLock = self.lock();
    if (!selfLock)
    {
      return;
    }

    if (!ec)
    {
      SignalMessage(message);
      ReadHeader();
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Reading body failed, dying: ", ec, ec.message());
      SignalLeave();
    }
  };

  if (socket)
  {
    assert(strand->running_in_this_thread());
    asio::async_read(*socket, asio::buffer(message.pointer(), message.size()), strand->wrap(cb));
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Invalid socket when attempting to read body");
    SignalLeave();
  }
}

void TCPClientImplementation::SetHeader(byte_array::ByteArray &header, uint64_t bufSize)
{
  header.Resize(16);

  for (std::size_t i = 0; i < 8; ++i)
  {
    header[i] = uint8_t((networkMagic_ >> i * 8) & 0xff);
  }

  for (std::size_t i = 0; i < 8; ++i)
  {
    header[i + 8] = uint8_t((bufSize >> i * 8) & 0xff);
  }
}

// Always executed in a run(), in a strand
void TCPClientImplementation::WriteNext(SharedSelfType const &selfLock)
{
  // Only one thread can get past here at a time. Effectively a try_lock
  // except that we can't unlock a mutex in the callback (undefined behaviour)
  {
    FETCH_LOCK(can_write_mutex_);
    if (can_write_)
    {
      can_write_ = false;
    }
    else
    {
      return;
    }
  }

  MessageType buffer;
  {
    FETCH_LOCK(queue_mutex_);
    if (write_queue_.empty())
    {
      FETCH_LOCK(can_write_mutex_);
      can_write_ = true;
      return;
    }
    buffer = write_queue_.front();
    write_queue_.pop_front();
  }

  byte_array::ByteArray header;
  SetHeader(header, buffer.size());

  std::vector<asio::const_buffer> buffers{asio::buffer(header.pointer(), header.size()),
                                          asio::buffer(buffer.pointer(), buffer.size())};

  auto socket = socket_.lock();

  auto cb = [this, selfLock, socket, buffer, header](std::error_code ec, std::size_t len) {
    FETCH_UNUSED(len);

    {
      FETCH_LOCK(can_write_mutex_);
      can_write_ = true;
    }

    if (ec)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Error writing to socket, closing.");
      SignalLeave();
    }
    else
    {
      // TODO(issue 16): this strand should be unnecessary
      auto strandLock = strand_.lock();
      if (strandLock)
      {
        WriteNext(selfLock);
      }
    }
  };

  auto strand = strand_.lock();

  if (socket && strand)
  {
    assert(strand->running_in_this_thread());
    asio::async_write(*socket, buffers, strand->wrap(cb));
  }
  else
  {
    if (!posted_close_)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to lock socket in WriteNext!");
    }

    SignalLeave();
  }
}

}  // namespace network
}  // namespace fetch
