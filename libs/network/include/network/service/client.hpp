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
class ServiceClient
    : public ServiceClientInterface
    , public ServiceServerInterface
    , public std::enable_shared_from_this<ServiceClient>
{
public:
  using network_manager_type = fetch::network::NetworkManager;
  using mutex_type = fetch::mutex::Mutex;
  using lock_type = std::lock_guard<mutex_type>;
  
  static constexpr char const *LOGGING_NAME = "ServiceClient";

  ServiceClient(std::shared_ptr<network::AbstractConnection> connection,
                const network_manager_type &                 network_manager)
    : connection_(connection)
    , network_manager_(network_manager)
    , message_mutex_(__LINE__, __FILE__)
  {
    auto connection_strong = connection_.lock();
    if (connection_strong)
    {
      connection_strong -> ActivateSelfManage();
    }
  }

  void Setup()
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Client::Setup() ",this," About to create a weak THIS");
    std::weak_ptr<ServiceClient> weakself;
    std::shared_ptr<ServiceClient> strongself;
    auto connection_strong = connection_.lock();
    if (connection_strong)
    {
      try {
        strongself = shared_from_this();
        if (strongself)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Client::Setup() ",this," strong seems OK");
        }
        weakself = std::weak_ptr<ServiceClient>(strongself);
        FETCH_LOG_WARN(LOGGING_NAME, "Client::Setup() ",this," created a weak THIS");
        connection_strong -> OnMessage([weakself](network::message_type const &msg) {
            LOG_STACK_TRACE_POINT;

            if (auto strongself = weakself.lock())
            {
              {
                std::lock_guard<fetch::mutex::Mutex> lock(strongself -> message_mutex_);
                strongself -> messages_.push_back(msg);
              }
              strongself -> ProcessMessages();
            }
          });
      }
      catch (const std::bad_weak_ptr& e) {
        FETCH_LOG_WARN(LOGGING_NAME, "Client::Setup() ",this," failed to create weak THIS -- suspect deleted somewhere?");
      }
    }
  }

  ServiceClient(network::TCPClient &connection, network_manager_type thread_manager)
    : ServiceClient(connection.connection_pointer().lock(), thread_manager)
  {}

  ~ServiceClient()
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Client::~Client", this);
    lock_type lock(deletion_safety_mutex_);
    LOG_STACK_TRACE_POINT;
    auto connection_strong = connection_.lock();
    if (connection_strong)
    {
      connection_strong -> OnMessage(nullptr);
      connection_strong -> Close();
      connection_strong -> ClearClosures();
    }
  }

  void Close()
  {
    auto connection_strong = connection_.lock();
    if (connection_strong)
    {
      connection_strong -> Close();
    }
  }

  connection_handle_type handle() const
  {
    auto connection_strong = connection_.lock();
    if (connection_strong)
    {
      return connection_strong -> handle();
    }
    LOG_STACK_TRACE_POINT;
    TODO_FAIL("connection is dead in ServiceClient::handle");
  }

  bool is_alive() const
  {
    auto connection_strong = connection_.lock();
    if (connection_strong)
    {
      return connection_strong -> is_alive();
    }
    return false;
  }

  uint16_t Type() const
  {
    auto connection_strong = connection_.lock();
    if (connection_strong)
    {
      return connection_strong -> Type();
    }
    return uint16_t(-1);
  }

  std::shared_ptr<network::AbstractConnection> connection() { return connection_.lock(); }

protected:
  bool DeliverRequest(network::message_type const &msg) override
  {
    auto connection_strong = connection_.lock();
    if (connection_strong)
    {
      if (connection_strong -> Closed()) return false;

      connection_strong -> Send(msg);
      return true;
    }

    return false;
  }

  bool DeliverResponse(connection_handle_type, network::message_type const &msg) override
  {
    auto connection_strong = connection_.lock();
    if (connection_strong)
    {
      if (connection_strong -> Closed()) return false;

      connection_strong -> Send(msg);
      return true;
    }

    return false;
  }

private:
  std::weak_ptr<network::AbstractConnection> connection_;

  void ProcessMessages()
  {
    FETCH_LOG_WARN("Client::ProcessMessages", this);
    try
    {
      lock_type lock(deletion_safety_mutex_);
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
            FETCH_LOG_DEBUG(LOGGING_NAME,"Looking for RPC functionality");

            if (!PushProtocolRequest(connection_handle_type(-1), msg))
            {
              throw serializers::SerializableException(error::UNKNOWN_MESSAGE,
                                                       byte_array::ConstByteArray("Unknown message"));
            }
          }
        }
      }
    }
    catch (...)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,"OMG, bad lock in ProcessMessages", this);
    }
  }

  network_manager_type                        network_manager_;
  std::deque<network::message_type>           messages_;
  mutable fetch::mutex::Mutex                 message_mutex_;
  mutable mutex_type deletion_safety_mutex_{__LINE__, __FILE__};
};
}  // namespace service
}  // namespace fetch
