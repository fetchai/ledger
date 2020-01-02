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

#include "network/tcp/client_connection.hpp"
#include "network/tcp/tcp_server.hpp"

#include <chrono>
#include <exception>
#include <memory>
#include <new>
#include <system_error>
#include <thread>
#include <utility>

namespace fetch {
namespace network {

TCPServer::TCPServer(uint16_t port, NetworkManagerType const &network_manager)
  : network_manager_{network_manager}
  , port_{port}
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Creating TCP server on tcp://0.0.0.0:", port);

  manager_ = std::make_shared<ClientManager>(*this);
}

TCPServer::~TCPServer()
{
  std::weak_ptr<AcceptorType> acceptorWeak = acceptor_;

  network_manager_.Post([acceptorWeak] {
    auto acceptorStrong = acceptorWeak.lock();
    if (acceptorStrong)
    {
      std::error_code ec;
      acceptorStrong->close(ec);

      if (ec)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Closed TCP server server acceptor: ", ec.message());
      }
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to close acceptor");
    }
  });

  // Need to block until the acceptor has expired as it refers back to this class.
  while (!acceptor_.expired())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}

void TCPServer::Start()
{
  std::shared_ptr<int> closure_alive = std::make_shared<int>(-1);

  {
    auto closure = [this, closure_alive] {
      std::shared_ptr<AcceptorType> acceptor;
      FETCH_LOG_DEBUG(LOGGING_NAME, "Opening TCP server");

      try
      {
        acceptor = network_manager_.CreateIO<AcceptorType>(
            asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_));

        acceptor_ = acceptor;

        // always update the port with the "correct" listening port
        port_ = acceptor->local_endpoint().port();

        FETCH_LOG_DEBUG(LOGGING_NAME, "Starting TCP server acceptor loop");

        if (acceptor)
        {
          Accept(acceptor);

          FETCH_LOG_DEBUG(LOGGING_NAME, "Accepting TCP server connections");
        }
      }
      catch (std::exception const &e)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Failed to open socket: ", port_, " with error: ", e.what());
      }

      counter_.Completed();
    };

    if (!network_manager_.Running())
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "TCP server trying to start with non-running network manager. This will block "
                     "until the network manager starts!");
    }

    network_manager_.Post(closure);
  }  // end of scope for closure which would hold closure_alive.use_count() at 2

  // Guarantee posted closure has either executed or will never execute
  for (std::size_t loop = 0; closure_alive.use_count() != 1; ++loop)
  {
    // allow start up time before showing warnings
    if (loop > 5)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "TCP server is waiting to open. Use count: ", closure_alive.use_count(),
                     " NM running: ", network_manager_.Running());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Listening for incoming connections on tcp://0.0.0.0:", port_);
}

void TCPServer::Stop()
{}

uint16_t TCPServer::GetListeningPort() const
{
  return port_;
}

void TCPServer::PushRequest(ConnectionHandleType client, MessageBuffer const &msg)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Got request from ", client);

  FETCH_LOCK(request_mutex_);
  requests_.push_back({client, msg});
}

void TCPServer::Broadcast(MessageBuffer const &msg)
{
  manager_->Broadcast(msg);
}

bool TCPServer::Send(ConnectionHandleType const &client, MessageBuffer const &msg)
{
  return manager_->Send(client, msg);
}

bool TCPServer::has_requests()
{
  FETCH_LOCK(request_mutex_);
  return !requests_.empty();
}

TCPServer::Request TCPServer::Top()
{
  FETCH_LOCK(request_mutex_);
  Request top = requests_.front();
  return top;
}

void TCPServer::Pop()
{
  FETCH_LOCK(request_mutex_);
  requests_.pop_front();
}

std::string TCPServer::GetAddress(ConnectionHandleType const &client)
{
  return manager_->GetAddress(client);
}

void TCPServer::Accept(std::shared_ptr<asio::ip::tcp::tcp::acceptor> const &acceptor)
{
  auto strongSocket                = network_manager_.CreateIO<asio::ip::tcp::tcp::socket>();
  std::weak_ptr<ClientManager> man = manager_;

  auto cb = [this, man, acceptor, strongSocket](std::error_code ec) {
    auto lock_ptr = man.lock();
    if (!lock_ptr)
    {
      return;
    }

    if (!ec)
    {
      auto conn = std::make_shared<ClientConnection>(strongSocket, manager_, network_manager_);
      auto ptr  = connection_register_.lock();

      if (ptr)
      {
        ptr->Enter(conn->connection_pointer());
        conn->SetConnectionManager(ptr);
      }

      conn->Start();
      Accept(acceptor);
    }
    else if (asio::error::operation_aborted == ec.value())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Shutting down server on tcp://0.0.0.0:", port_);
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Error generated in acceptor: ", ec.message());
    }
  };

  acceptor->async_accept(*strongSocket, cb);
}

}  // namespace network
}  // namespace fetch
