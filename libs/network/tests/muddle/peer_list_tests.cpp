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

#include "network/muddle/dispatcher.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/muddle_register.hpp"
#include "network/muddle/peer_list.hpp"
#include "network/muddle/router.hpp"
#include "network/service/promise.hpp"

#include "gmock/gmock.h"

#include <chrono>
#include <memory>
#include <thread>

using fetch::muddle::NetworkId;

namespace fetch {
namespace muddle {

struct DevNull : public network::AbstractConnection
{
  virtual void Send(network::message_type const &) override
  {}

  virtual uint16_t Type() const override
  {
    return 0xFFFF;
  }

  virtual void Close() override
  {}

  virtual bool Closed() const override
  {
    return false;
  }

  virtual bool is_alive() const override
  {
    return true;
  }
};

}  // namespace muddle
}  // namespace fetch

class PeerConnectionListTests : public ::testing::Test
{
protected:
  using Dispatcher         = fetch::muddle::Dispatcher;
  using MuddleRegister     = fetch::muddle::MuddleRegister;
  using Router             = fetch::muddle::Router;
  using PeerConnectionList = fetch::muddle::PeerConnectionList;
  using ConnectionPtr      = PeerConnectionList::ConnectionPtr;
  using ConnectionState    = PeerConnectionList::ConnectionState;
  using Uri                = fetch::network::Uri;
  using Peer               = fetch::network::Peer;

  PeerConnectionListTests()
    : register_(dispatcher_)
    , router_(NetworkId{"Test"},
              Router::Address{"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"},
              register_, dispatcher_)
    , peer_list_(router_)
    , peer_(Peer{"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", 42})
    , connection_(::std::make_shared<fetch::muddle::DevNull>())
  {}

  void SetUp() override
  {}

  Dispatcher         dispatcher_;
  MuddleRegister     register_;
  Router             router_;
  PeerConnectionList peer_list_;
  Uri                peer_;
  ConnectionPtr      connection_;
};

TEST_F(PeerConnectionListTests, CheckDisconnect)
{
  EXPECT_EQ(peer_list_.GetNumPeers(), 0);
  EXPECT_EQ(peer_list_.GetCurrentPeers().size(), 0);
  EXPECT_EQ(peer_list_.GetStateForPeer(peer_), ConnectionState::UNKNOWN);

  peer_list_.AddConnection(peer_, connection_);
  EXPECT_EQ(peer_list_.GetNumPeers(), 0);
  EXPECT_EQ(peer_list_.GetCurrentPeers().size(), 1);
  EXPECT_EQ(peer_list_.GetStateForPeer(peer_), ConnectionState::TRYING);

  peer_list_.OnConnectionEstablished(peer_);
  EXPECT_EQ(peer_list_.GetStateForPeer(peer_), ConnectionState::CONNECTED);

  peer_list_.RemoveConnection(peer_);
  EXPECT_EQ(peer_list_.GetStateForPeer(peer_), ConnectionState(int(ConnectionState::BACKOFF) + 1));
  {
    using namespace std::chrono_literals;
    ::std::this_thread::sleep_for(2s);
  }
  peer_list_.OnConnectionEstablished(peer_);
  EXPECT_EQ(peer_list_.GetStateForPeer(peer_), ConnectionState::CONNECTED);

  peer_list_.Disconnect(peer_);
  EXPECT_EQ(peer_list_.GetStateForPeer(peer_), ConnectionState::UNKNOWN);
}
