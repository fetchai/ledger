#pragma once
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

#include "fake_muddle_endpoint.hpp"
#include "muddle/muddle_endpoint.hpp"

#include "gmock/gmock.h"

class MockMuddleEndpoint : public fetch::muddle::MuddleEndpoint
{
public:
  using Address   = fetch::muddle::Address;
  using NetworkId = fetch::muddle::NetworkId;

  MockMuddleEndpoint(Address address, NetworkId const &network_id)
    : fake{std::move(address), network_id}
  {
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(*this, GetAddress()).WillByDefault(Invoke(&fake, &FakeMuddleEndpoint::GetAddress));
    ON_CALL(*this, Subscribe(_, _))
        .WillByDefault(
            Invoke<FakeMuddleEndpoint, SubscriptionPtr (FakeMuddleEndpoint::*)(uint16_t, uint16_t)>(
                &fake, &FakeMuddleEndpoint::Subscribe));
    ON_CALL(*this, Subscribe(_, _, _))
        .WillByDefault(Invoke<FakeMuddleEndpoint, SubscriptionPtr (FakeMuddleEndpoint::*)(
                                                      Address const &, uint16_t, uint16_t)>(
            &fake, &FakeMuddleEndpoint::Subscribe));
    ON_CALL(*this, network_id()).WillByDefault(Invoke(&fake, &FakeMuddleEndpoint::network_id));
  }

  MOCK_CONST_METHOD0(GetAddress, Address const &());
  MOCK_METHOD4(Send, void(Address const &, uint16_t, uint16_t, Payload const &));
  MOCK_METHOD5(Send, void(Address const &, uint16_t, uint16_t, Payload const &, Options));
  MOCK_METHOD5(Send, void(Address const &, uint16_t, uint16_t, uint16_t, Payload const &));
  MOCK_METHOD6(Send, void(Address const &, uint16_t, uint16_t, uint16_t, Payload const &, Options));
  MOCK_METHOD3(Broadcast, void(uint16_t, uint16_t, Payload const &));
  MOCK_METHOD4(Exchange, Response(Address const &, uint16_t, uint16_t, Payload const &));
  MOCK_METHOD2(Subscribe, SubscriptionPtr(uint16_t, uint16_t));
  MOCK_METHOD3(Subscribe, SubscriptionPtr(Address const &, uint16_t, uint16_t));
  MOCK_CONST_METHOD0(network_id, NetworkId const &());
  MOCK_CONST_METHOD0(GetDirectlyConnectedPeers, AddressList());
  MOCK_CONST_METHOD0(GetDirectlyConnectedPeerSet, AddressSet());

  FakeMuddleEndpoint fake;
};
