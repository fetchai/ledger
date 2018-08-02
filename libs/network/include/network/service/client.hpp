#pragma once

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
    : connection_(connection)
    , network_manager_(network_manager)
    , message_mutex_(__LINE__, __FILE__)
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

        // Since this class isn't shared_from_this, try to ensure safety when
        // destructing
        network_manager_.Post([this]() { ProcessMessages(); });
      });
    }

    /*
    ptr->OnConnectionFailed([this]() {
        // TODO: Clear closures?
      });
    */
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

      // Disconnect callbacks
      if (ptr->Closed())
      {
        ptr->ClearClosures();
        ptr->Close();

        int timeout = 100;

        // Can only guarantee we are not being called when socket is closed
        while (!ptr->Closed())
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          timeout--;

          if (timeout == 0) break;
        }
      }
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
        // TODO: Post
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
