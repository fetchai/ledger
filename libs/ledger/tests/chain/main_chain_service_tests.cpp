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

#include "gtest/gtest.h"
#include "ledger/chain/main_chain.hpp"
#include "crypto/ecdsa.hpp"
#include "chain/address.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "mock_consensus.hpp"
#include "mock_main_chain_rpc_client.hpp"
#include "mock_muddle_endpoint.hpp"
#include "mock_trust_system.hpp"
#include "ledger/testing/block_generator.hpp"
#include "muddle/network_id.hpp"
#include "core/serializers/main_serializer.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using fetch::ledger::MainChainRpcService;
using fetch::ledger::MainChain;
using fetch::crypto::ECDSASigner;
using fetch::chain::Address;
using fetch::muddle::NetworkId;
using fetch::ledger::testing::BlockGenerator;
using fetch::ledger::BlockStatus;
using fetch::ledger::MainChainProtocol;
using fetch::chain::GetGenesisDigest;
using fetch::serializers::LargeObjectSerializeHelper;

using AddressList   = fetch::muddle::MuddleEndpoint::AddressList;
using State         = MainChainRpcService::State;
using MuddleAddress = fetch::muddle::Address;

std::ostream &operator<<(std::ostream &s, MainChainRpcService::State state)
{
  s << fetch::ledger::ToString(state);
  return s;
}

namespace {

template <typename T>
std::shared_ptr<T> CreateNonOwning(T &value)
{
  return std::shared_ptr<T>(&value, [](T *) {});
}

template <typename T>
fetch::network::PromiseOf<T> CreatePromise(T const &item)
{
  LargeObjectSerializeHelper serializer;
  serializer << item;

  // populate a successful promise
  auto prom = fetch::service::MakePromise();
  prom->Fulfill(serializer.data());

  return fetch::network::PromiseOf<T>{prom};
}

constexpr std::size_t NUM_LANES  = 1;
constexpr std::size_t NUM_SLICES = 16;

class MainChainServiceTests : public ::testing::Test
{
protected:

  static void SetUpTestCase()
  {
    fetch::crypto::mcl::details::MCLInitialiser();
    fetch::chain::InitialiseTestConstants();
  }

  void Tick(State current_state, State next_state);

  BlockGenerator                   block_generator_{NUM_LANES, NUM_SLICES};
  ECDSASigner                      self_;
  ECDSASigner                      other1_signer_;
  ECDSASigner                      other2_signer_;
  ECDSASigner                      other3_signer_;
  MuddleAddress                    other1_{other1_signer_.identity().identifier()};
  MuddleAddress                    other2_{other2_signer_.identity().identifier()};
  MuddleAddress                    other3_{other3_signer_.identity().identifier()};
  NiceMock<MockMainChainRpcClient> rpc_client_;
  NiceMock<MockMuddleEndpoint>     endpoint_{self_.identity().identifier(), NetworkId{"TEST"}};
  NiceMock<MockConsensus>          consensus_;
  NiceMock<MockTrustSystem>        trust_;
  MainChain                        chain_;
  MainChainRpcService              rpc_service_{endpoint_,
                                   rpc_client_,
                                   chain_,
                                   trust_,
                                   MainChainRpcService::Mode::STANDALONE,
                                   CreateNonOwning(consensus_)};


};

void MainChainServiceTests::Tick(State current_state, State next_state)
{
  ASSERT_EQ(rpc_service_.state(), current_state);
  auto sm = rpc_service_.GetWeakRunnable().lock();
  sm->Execute();
  ASSERT_EQ(rpc_service_.state(), next_state);
}

TEST_F(MainChainServiceTests, CheckNoPeersCase)
{
  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{}));

  Tick(State::SYNCHRONISED, State::REQUEST_HEAVIEST_CHAIN);
  Tick(State::REQUEST_HEAVIEST_CHAIN, State::SYNCHRONISED);
}

TEST_F(MainChainServiceTests, CheckSimpleCatchUpFromSinglePeer)
{
  auto gen = block_generator_();
  auto b1 = block_generator_(gen);
  auto b2 = block_generator_(b1);
  auto b3 = block_generator_(b2);
  auto b4 = block_generator_(b3);

  MainChain other1_chain;
  MainChainProtocol other1_proto{other1_chain};
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b1));
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b2));
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b3));
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b4));

  auto travelogue = other1_proto.TimeTravel(GetGenesisDigest());

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest())).WillOnce(Return(CreatePromise(travelogue)));

  Tick(State::SYNCHRONISED, State::REQUEST_HEAVIEST_CHAIN);
  Tick(State::REQUEST_HEAVIEST_CHAIN, State::WAIT_FOR_HEAVIEST_CHAIN);
  Tick(State::WAIT_FOR_HEAVIEST_CHAIN, State::SYNCHRONISED);
}

}