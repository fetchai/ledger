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

#include "core/assert.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/logging.hpp"
#include "core/mutex.hpp"
#include "core/serializers/exception.hpp"
#include "network/message.hpp"
#include "network/service/client_interface.hpp"
#include "network/service/error_codes.hpp"
#include "network/service/server_interface.hpp"

#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <utility>

namespace fetch {
namespace service {

template <typename T>
class ServiceServer : public T, public ServiceServerInterface
{
public:
  using super_type = T;
  using self_type  = ServiceServer<T>;

  using network_manager_type = typename super_type::network_manager_type;
  using handle_type          = typename T::connection_handle_type;

  static constexpr char const *LOGGING_NAME = "ServiceServer";

  // TODO(issue 20): Rename and move
  class ClientRPCInterface : public ServiceClientInterface
  {
  public:
    ClientRPCInterface() = delete;

    ClientRPCInterface(ClientRPCInterface const &) = delete;
    ClientRPCInterface const &operator=(ClientRPCInterface const &) = delete;

    ClientRPCInterface(self_type *server, handle_type client)
      : server_(server)
      , client_(client)
    {}

    bool ProcessMessage(network::message_type const &msg)
    {
      return this->ProcessServerMessage(msg);
    }

  protected:
    bool DeliverRequest(network::message_type const &msg) override
    {
      server_->Send(client_, msg);
      return true;
    }

  private:
    self_type *server_;  // TODO(issue 20): Change to shared ptr and add
                         // enable_shared_from_this on service
    handle_type client_;
  };
  // EN of ClientRPC

  struct PendingMessage
  {
    handle_type           client;
    network::message_type message;
  };
  using byte_array_type = byte_array::ConstByteArray;

  ServiceServer(uint16_t port, network_manager_type network_manager)
    : super_type(port, network_manager)
    , network_manager_(network_manager)
    , message_mutex_(__LINE__, __FILE__)
  {}

  ~ServiceServer()
  {
    FETCH_LOCK(client_rpcs_mutex_);

    for (auto &c : client_rpcs_)
    {
      delete c.second;
    }
  }

  ClientRPCInterface &ServiceInterfaceOf(handle_type const &i)
  {
    FETCH_LOCK(client_rpcs_mutex_);

    if (client_rpcs_.find(i) == client_rpcs_.end())
    {
      // TODO(issue 20): Make sure to delete this on disconnect after all promises has
      // been fulfilled
      client_rpcs_.emplace(std::make_pair(i, new ClientRPCInterface(this, i)));
    }

    return *client_rpcs_[i];
  }

protected:
  bool DeliverResponse(handle_type client, network::message_type const &msg) override
  {
    return super_type::Send(client, msg);
  }

private:
  void PushRequest(handle_type client, network::message_type const &msg) override
  {
    FETCH_LOCK(message_mutex_);
    FETCH_LOG_DEBUG(LOGGING_NAME, "RPC call from ", client);
    PendingMessage pm = {client, msg};
    messages_.push_back(pm);

    // TODO(issue 20): look at this
    network_manager_.Post([this]() { this->ProcessMessages(); });
  }

  void ProcessMessages()
  {
    bool has_messages = false;
    {
      FETCH_LOCK(message_mutex_);
      has_messages = (!messages_.empty());
    }

    while (has_messages)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "MESSAGES!!!!");

      PendingMessage pm;
      {
        FETCH_LOCK(message_mutex_);

        FETCH_LOG_DEBUG(LOGGING_NAME, "Server side backlog: ", messages_.size());
        has_messages = (!messages_.empty());
        if (has_messages)
        {  // To ensure we can make a worker pool in the future
          pm = messages_.front();
          messages_.pop_front();
        }
      }

      if (has_messages)
      {
        network_manager_.Post([this, pm]() {
          FETCH_LOG_DEBUG(LOGGING_NAME, "Processing message call", pm.client, ":", pm.message);

          if (!this->PushProtocolRequest(pm.client, pm.message))
          {
            bool processed = false;

            FETCH_LOG_INFO(LOGGING_NAME, "PushProtocolRequest returned FALSE!");

            {
              FETCH_LOCK(client_rpcs_mutex_);
              if (client_rpcs_.find(pm.client) != client_rpcs_.end())
              {
                auto &c   = client_rpcs_[pm.client];
                processed = c->ProcessMessage(pm.message);
              }
            }

            if (!processed)
            {
              // TODO(issue 20): Lookup client RPC handler
              FETCH_LOG_ERROR(LOGGING_NAME, "Possibly a response to a client?");

              throw serializers::SerializableException(
                  error::UNKNOWN_MESSAGE, byte_array::ConstByteArray("Unknown message"));
              TODO_FAIL("call type not implemented yet");
            }
          }
        });
      }
    }
  }

  network_manager_type network_manager_;

  std::deque<PendingMessage> messages_;
  mutable Mutex              message_mutex_{__LINE__, __FILE__};

  mutable Mutex                               client_rpcs_mutex_{__LINE__, __FILE__};
  std::map<handle_type, ClientRPCInterface *> client_rpcs_;
};
}  // namespace service
}  // namespace fetch
