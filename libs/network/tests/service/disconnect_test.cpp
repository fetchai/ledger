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

#include <chrono>
#include <memory>
#include <thread>

#include <gmock/gmock.h>

#include <network/muddle/dispatcher.hpp>
#include <network/muddle/muddle_register.hpp>
#include <network/muddle/peer_list.hpp>
#include <network/muddle/router.hpp>
#include <network/service/client_interface.hpp>
#include <network/service/promise.hpp>
#include <network/service/server.hpp>

class AlmostClient : public fetch::service::ServiceClientInterface
{
  bool disconnected_ = false;

protected:
  bool DeliverRequest(fetch::network::message_type const &)
  {
    return true;
  }
  void Disconnect()
  {
    disconnected_ = true;
  }

public:
  bool Disconnected() const
  {
    return disconnected_;
  }
  bool ProcessMessage(fetch::network::message_type const &msg)
  {
    return ProcessServerMessage(msg);
  }
};

class NotAServer
{
  AlmostClient client_;

public:
  using connection_handle_type = fetch::network::AbstractConnection::connection_handle_type;
  struct network_manager_type
  {
    template <class F>
    void Post(F &&)
    {}
  };

  static constexpr connection_handle_type the_only_handle = 42;

  NotAServer(uint16_t, network_manager_type)
  {}
  bool Send(connection_handle_type handle, fetch::network::message_type const &msg)
  {
    return handle == the_only_handle && (client_.ProcessMessage(msg), client_.Disconnected());
  }

protected:
  virtual void PushRequest(connection_handle_type, fetch::network::message_type const &) = 0;
};

using AlmostServer = fetch::service::ServiceServer<NotAServer>;

TEST(DisconnectTest, TryDisconnect)
{
  ASSERT_TRUE(AlmostServer(8888, NotAServer::network_manager_type{})
                  .Disconnect(NotAServer::the_only_handle));
}
