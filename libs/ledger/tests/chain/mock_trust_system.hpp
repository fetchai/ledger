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

#include "muddle/address.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"

#include "gmock/gmock.h"

class MockTrustSystem : public fetch::p2p::P2PTrustInterface<fetch::muddle::Address>
{
public:
  using MuddleAddress  = fetch::muddle::Address;
  using TrustSubject   = fetch::p2p::TrustSubject;
  using TrustQuality   = fetch::p2p::TrustQuality;
  using ConstByteArray = fetch::byte_array::ConstByteArray;

  MOCK_METHOD3(AddFeedback, void(MuddleAddress const &, TrustSubject, TrustQuality));
  MOCK_METHOD4(AddFeedback,
               void(MuddleAddress const &, ConstByteArray const &, TrustSubject, TrustQuality));

  MOCK_CONST_METHOD1(GetBestPeers, IdentitySet(std::size_t));
  MOCK_CONST_METHOD0(GetPeersAndTrusts, PeerTrusts());
  MOCK_CONST_METHOD2(GetRandomPeers, IdentitySet(std::size_t, double));
  MOCK_CONST_METHOD1(GetRankOfPeer, std::size_t(MuddleAddress const &));
  MOCK_CONST_METHOD1(GetTrustRatingOfPeer, double(MuddleAddress const &));
  MOCK_CONST_METHOD1(IsPeerTrusted, bool(MuddleAddress const &));
  MOCK_CONST_METHOD1(IsPeerKnown, bool(MuddleAddress const &));
  MOCK_CONST_METHOD0(Debug, void());
};
