#ifndef SERVICE_SERVICE_CLIENT_HPP
#define SERVICE_SERVICE_CLIENT_HPP

#include "core/serializers/referenced_byte_array.hpp"
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

namespace fetch {
namespace service {

template <typename T>
class ServiceClient : public T,
                      public ServiceClientInterface,
                      public ServiceServerInterface {
 public:
  typedef T super_type;

  typedef typename super_type::thread_manager_type thread_manager_type;
  typedef typename thread_manager_type::event_handle_type event_handle_type;

  ServiceClient(byte_array::ConstByteArray const& host, uint16_t const& port,
                thread_manager_type thread_manager)
      : super_type(thread_manager),
        thread_manager_(thread_manager),
        message_mutex_(__LINE__, __FILE__) {
    LOG_STACK_TRACE_POINT;

    this->Connect(host, port);
  }

  ~ServiceClient()
  {
    LOG_STACK_TRACE_POINT;

    // Disconnect callbacks
    super_type::Cleanup();
    super_type::Close();

    // Can only guarantee we are not being called when socket is closed
    while(!super_type::Closed())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  void PushMessage(network::message_type const& msg) override {
    LOG_STACK_TRACE_POINT;

    {
      std::lock_guard<fetch::mutex::Mutex> lock(message_mutex_);
      messages_.push_back(msg);
    }

    // Since this class isn't shared_from_this, try to ensure safety when destructing
    thread_manager_.Post([this]()
    {
      ProcessMessages();
    });
  }

  void ConnectionFailed() override {
    LOG_STACK_TRACE_POINT;

    this->ClearPromises();
  }

 protected:
  bool DeliverRequest(network::message_type const& msg) override {
    if (!super_type::is_alive()) return false;

    super_type::Send(msg);
    return true;
  }

  bool DeliverResponse(handle_type, network::message_type const& msg) override {
    super_type::Send(msg);
    return true;
  }

 private:
  void ProcessMessages() {
    LOG_STACK_TRACE_POINT;

    message_mutex_.lock();
    bool has_messages = (!messages_.empty());
    message_mutex_.unlock();

    while (has_messages) {
      message_mutex_.lock();

      network::message_type msg;
      has_messages = (!messages_.empty());
      if (has_messages) {
        msg = messages_.front();
        messages_.pop_front();
      };
      message_mutex_.unlock();

      if (has_messages) {
        // TODO: Post
        if (!ProcessServerMessage(msg)) {
          fetch::logger.Debug("Looking for RPC functionality");

          if (!PushProtocolRequest(handle_type(-1), msg)) {
            throw serializers::SerializableException(
                error::UNKNOWN_MESSAGE,
                byte_array::ConstByteArray("Unknown message"));
          }
        }
      }
    }
  }

  thread_manager_type               thread_manager_;
  std::deque<network::message_type> messages_;
  mutable fetch::mutex::Mutex       message_mutex_;
  //  std::thread *worker_thread_ = nullptr;  // TODO: use thread pool
};
}
}

#endif
