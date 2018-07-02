#ifndef SERVICE_SERVER_HPP
#define SERVICE_SERVER_HPP

#include "core/serializers/referenced_byte_array.hpp"
#include "core/serializers/serializable_exception.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/message_types.hpp"
#include "network/service/protocol.hpp"

#include "core/mutex.hpp"
#include "network/service/client_interface.hpp"
#include "network/service/error_codes.hpp"
#include "network/service/promise.hpp"
#include "network/service/server_interface.hpp"

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "network/tcp/tcp_server.hpp"

#include <atomic>
#include <deque>
#include <map>
#include <mutex>

namespace fetch {
namespace service {

template <typename T>
class ServiceServer : public T, public ServiceServerInterface {
 public:
  typedef T super_type;
  typedef ServiceServer<T> self_type;

  typedef typename super_type::thread_manager_type thread_manager_type;
  typedef typename thread_manager_type::event_handle_type event_handle_type;
  typedef typename T::handle_type handle_type;

  // TODO Rename and move
  class ClientRPCInterface : public ServiceClientInterface {
   public:
    ClientRPCInterface() = delete;

    ClientRPCInterface(ClientRPCInterface const&) = delete;
    ClientRPCInterface const& operator=(ClientRPCInterface const&) = delete;

    ClientRPCInterface(self_type* server, handle_type client)
        : server_(server), client_(client) {}

    bool ProcessMessage(network::message_type const& msg) {
      return this->ProcessServerMessage(msg);
    }

   protected:
    bool DeliverRequest(network::message_type const& msg) override {
      server_->Send(client_, msg);
      return true;
    }

   private:
    self_type* server_;  // TODO: Change to shared ptr and add
                         // enable_shared_from_this on service
    handle_type client_;
  };
  // EN of ClientRPC

  struct PendingMessage {
    handle_type client;
    network::message_type message;
  };
  typedef byte_array::ConstByteArray byte_array_type;

  class Oink
  {
  public:
    Oink(const char *msg)
    {
      std::cout << msg << std::endl;
    }
  };

  ServiceServer(uint16_t port, thread_manager_type thread_manager)
    :   super_type(port, thread_manager),
        oink1("OINK1111111!"),
        thread_manager_(thread_manager),
        oink2("OINK22222222!"),
        message_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;
  }

  ~ServiceServer() {
    LOG_STACK_TRACE_POINT;

    client_rpcs_mutex_.lock();

    for (auto& c : client_rpcs_) {
      delete c.second;
    }

    client_rpcs_mutex_.unlock();
  }

  ClientRPCInterface& ServiceInterfaceOf(handle_type const& i) {
    std::lock_guard<fetch::mutex::Mutex> lock(client_rpcs_mutex_);

    if (client_rpcs_.find(i) == client_rpcs_.end()) {
      // TODO: Make sure to delete this on disconnect after all promises has
      // been fulfilled
      client_rpcs_.emplace(std::make_pair(i, new ClientRPCInterface(this, i)));
    }

    return *client_rpcs_[i];
  }

 protected:
  bool DeliverResponse(handle_type client,
                       network::message_type const& msg) override {
    return super_type::Send(client, msg);
  }

 private:
  void PushRequest(handle_type client,
                   network::message_type const& msg) override {
    LOG_STACK_TRACE_POINT;

    std::lock_guard<fetch::mutex::Mutex> lock(message_mutex_);
    fetch::logger.Info("RPC call from ", client);
    PendingMessage pm = {client, msg};
    messages_.push_back(pm);

    // TODO: (`HUT`) : look at this
    thread_manager_.Post([this]() { this->ProcessMessages(); });
  }

  void ProcessMessages() {
    LOG_STACK_TRACE_POINT;

    message_mutex_.lock();
    bool has_messages = (!messages_.empty());
    message_mutex_.unlock();

    while (has_messages) {
      message_mutex_.lock();

      PendingMessage pm;
      fetch::logger.Debug("Server side backlog: ", messages_.size());
      has_messages = (!messages_.empty());
      if (has_messages) {  // To ensure we can make a worker pool in the future
        pm = messages_.front();
        messages_.pop_front();
      };

      message_mutex_.unlock();

      if (has_messages) {
        thread_manager_.Post([this, pm]() {
          fetch::logger.Debug("Processing message call");
          if (!this->PushProtocolRequest(pm.client, pm.message)) {
            bool processed = false;

            client_rpcs_mutex_.lock();
            if (client_rpcs_.find(pm.client) != client_rpcs_.end()) {
              auto& c = client_rpcs_[pm.client];
              processed = c->ProcessMessage(pm.message);
            }
            client_rpcs_mutex_.unlock();

            if (!processed) {
              // TODO: Lookup client RPC handler
              fetch::logger.Error("Possibly a response to a client?");

              throw serializers::SerializableException(
                  error::UNKNOWN_MESSAGE,
                  byte_array::ConstByteArray("Unknown message"));
              TODO_FAIL("call type not implemented yet");
            }
          }

        });
      }
    }
  }

  Oink oink1;
  thread_manager_type thread_manager_;
  Oink oink2;

  std::deque<PendingMessage> messages_;
  mutable fetch::mutex::Mutex message_mutex_;

  mutable fetch::mutex::Mutex client_rpcs_mutex_;
  std::map<handle_type, ClientRPCInterface*> client_rpcs_;
};
}
}

#endif
