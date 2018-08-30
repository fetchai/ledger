#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/serializers/byte_array.hpp"
#include "core/serializers/serializable_exception.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/message_types.hpp"
#include "network/service/protocol.hpp"

#include "network/service/client_interface.hpp"
#include "network/service/error_codes.hpp"
#include "network/service/promise.hpp"
#include "network/service/server_interface.hpp"

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/tcp/tcp_client.hpp"

#include <map>
#include <utility>

namespace fetch {
namespace service {

// template <typename T>
class ServiceClient : public ServiceClientInterface, public ServiceServerInterface
{
public:
  using network_manager_type = fetch::network::NetworkManager;

  ServiceClient(std::shared_ptr<network::AbstractConnection> connection,
                network_manager_type const &                 network_manager)
    : connection_(connection), network_manager_(network_manager), message_mutex_(__LINE__, __FILE__)
  {
    auto ptr = connection_.lock();
    if (ptr)
    {
      ptr->ActivateSelfManage();

      ptr->OnMessage([this](network::message_type const &msg) {
        LOG_STACK_TRACE_POINT;

        {
          std::lock_guard<fetch::mutex::Mutex> lock(message_mutex_);
          messages_.push_back(msg);
        }

        ProcessMessages();
      });
    }
  }

  ServiceClient(network::TCPClient &connection, network_manager_type thread_manager)
    : ServiceClient(connection.connection_pointer().lock(), thread_manager)
  {}

  ~ServiceClient()
  {
    LOG_STACK_TRACE_POINT;

    auto ptr = connection_.lock();

    if (ptr)
    {
      ptr->ClearClosures();
      ptr->Close();
    }
  }

  void Close()
  {
    auto ptr = connection_.lock();
    if (ptr)
    {
      ptr->Close();
    }
  }

  connection_handle_type handle() const
  {
    auto ptr = connection_.lock();
    if (ptr)
    {
      return ptr->handle();
    }
    TODO_FAIL("connection is dead");
  }

  bool is_alive() const
  {
    auto ptr = connection_.lock();
    if (ptr)
    {
      return ptr->is_alive();
    }
    return false;
  }

  bool WaitForAlive(std::size_t milliseconds) const
  {
    auto ptr = connection_.lock();

    if (ptr)
    {
      for (std::size_t i = 0; i < milliseconds; i += 10)
      {
        if (ptr->is_alive())
        {
          return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
    return false;
  }

  uint16_t Type() const
  {
    auto ptr = connection_.lock();
    if (ptr)
    {
      return ptr->Type();
    }
    return uint16_t(-1);
  }

  std::shared_ptr<network::AbstractConnection> connection() { return connection_.lock(); }

protected:
  bool DeliverRequest(network::message_type const &msg) override
  {
    auto ptr = connection_.lock();
    if (ptr)
    {
      if (ptr->Closed()) return false;

      ptr->Send(msg);
      return true;
    }

    return false;
  }

  bool DeliverResponse(connection_handle_type, network::message_type const &msg) override
  {
    auto ptr = connection_.lock();
    if (ptr)
    {
      if (ptr->Closed()) return false;

      ptr->Send(msg);
      return true;
    }

    return false;
  }

private:
  std::weak_ptr<network::AbstractConnection> connection_;
  void                                       ProcessMessages()
  {
    LOG_STACK_TRACE_POINT;

    message_mutex_.lock();
    bool has_messages = (!messages_.empty());
    message_mutex_.unlock();

    while (has_messages)
    {
      message_mutex_.lock();

      network::message_type msg;
      has_messages = (!messages_.empty());
      if (has_messages)
      {
        msg = messages_.front();
        messages_.pop_front();
      };
      message_mutex_.unlock();

      if (has_messages)
      {
        // TODO(issue 22): Post
        if (!ProcessServerMessage(msg))
        {
          fetch::logger.Debug("Looking for RPC functionality");

          if (!PushProtocolRequest(connection_handle_type(-1), msg))
          {
            throw serializers::SerializableException(error::UNKNOWN_MESSAGE,
                                                     byte_array::ConstByteArray("Unknown message"));
          }
        }
      }
    }
  }

  network_manager_type              network_manager_;
  std::deque<network::message_type> messages_;
  mutable fetch::mutex::Mutex       message_mutex_;
};
}  // namespace service
}  // namespace fetch
