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

#include "chain/address.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/ecdsa.hpp"
#include "gtest/gtest.h"
#include "ledger/chain/main_chain.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "ledger/testing/block_generator.hpp"
#include "mock_consensus.hpp"
#include "mock_main_chain_rpc_client.hpp"
#include "mock_muddle_endpoint.hpp"
#include "mock_trust_system.hpp"
#include "moment/clocks.hpp"
#include "muddle/network_id.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using fetch::ledger::MainChainRpcService;
using fetch::ledger::MainChain;
using fetch::crypto::ECDSASigner;
using fetch::muddle::NetworkId;
using fetch::ledger::testing::BlockGenerator;
using fetch::ledger::BlockStatus;
using fetch::ledger::MainChainProtocol;
using fetch::chain::GetGenesisDigest;
using fetch::serializers::LargeObjectSerializeHelper;
using fetch::ledger::ConsensusInterface;

using AddressList        = fetch::muddle::MuddleEndpoint::AddressList;
using State              = MainChainRpcService::State;
using MuddleAddress      = fetch::muddle::Address;
using TraveloguePromise  = fetch::network::PromiseOf<MainChainProtocol::Travelogue>;
using AdjustableClockPtr = fetch::moment::AdjustableClockPtr;

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

  AdjustableClockPtr               clock_{fetch::moment::CreateAdjustableClock("MC_RPC:main")};
  BlockGenerator                   block_generator_{NUM_LANES, NUM_SLICES};
  ECDSASigner                      self_;
  ECDSASigner                      other1_signer_;
  MuddleAddress                    other1_{other1_signer_.identity().identifier()};
  NiceMock<MockMainChainRpcClient> rpc_client_;
  NiceMock<MockMuddleEndpoint>     endpoint_{self_.identity().identifier(), NetworkId{"TEST"}};
  NiceMock<MockConsensus>          consensus_;
  NiceMock<MockTrustSystem>        trust_;
  MainChain                        chain_;
  MainChainRpcService              rpc_service_{endpoint_, rpc_client_, chain_, trust_,
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

  Tick(State::SYNCHRONISING, State::SYNCHRONISED);

  // should stay in sync'ed state
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
}

TEST_F(MainChainServiceTests, CheckSimpleCatchUpFromSinglePeer)
{
  auto gen = block_generator_();
  auto b1  = block_generator_(gen);
  auto b2  = block_generator_(b1);
  auto b3  = block_generator_(b2);
  auto b4  = block_generator_(b3);

  MainChain         other1_chain;
  MainChainProtocol other1_proto{other1_chain};
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b1));
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b2));
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b3));
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b4));

  auto travelogue = other1_proto.TimeTravel(GetGenesisDigest());

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(CreatePromise(travelogue)));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  Tick(State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), b4->hash);
}

TEST_F(MainChainServiceTests, ChecIncrementalCatchUp)
{
  auto gen = block_generator_();
  auto b1  = block_generator_(gen);
  auto b2  = block_generator_(b1);
  auto b3  = block_generator_(b2);
  auto b4  = block_generator_(b3);

  MainChain         other1_chain;
  MainChainProtocol other1_proto{other1_chain};
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b1));
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b2));
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b3));
  EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b4));

  auto travelogue1 = other1_proto.TimeTravel(GetGenesisDigest());
  travelogue1.blocks.resize(2);  // simulate large sync forward in time

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(CreatePromise(travelogue1)));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), b2->hash);

  auto travelogue2 = other1_proto.TimeTravel(b2->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, b2->hash))
      .WillOnce(Return(CreatePromise(travelogue2)));

  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  Tick(State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);

  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), b4->hash);
}

TEST_F(MainChainServiceTests, ForkWhenPeerHasLongerChain)
{
  auto gen  = block_generator_();
  auto b1   = block_generator_(gen);
  auto b2   = block_generator_(b1);
  auto b3   = block_generator_(b2);
  auto b4   = block_generator_(b3);
  auto b5_1 = block_generator_(b4);
  auto b6_1 = block_generator_(b5_1);

  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b1));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b2));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b3));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b4));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b5_1));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b6_1));

  auto b5_2 = block_generator_(b4);
  auto b6_2 = block_generator_(b5_2);
  auto b7_2 = block_generator_(b6_2);

  MainChain         other1_chain;
  MainChainProtocol other1_proto{other1_chain};
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b1));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b2));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b3));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b4));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b5_2));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b6_2));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b7_2));

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  auto const log1 = other1_proto.TimeTravel(b5_1->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, b5_1->hash)).WillOnce(Return(CreatePromise(log1)));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  auto const log2 = other1_proto.TimeTravel(b4->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, b4->hash)).WillOnce(Return(CreatePromise(log2)));

  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  Tick(State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), b7_2->hash);

  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
}

TEST_F(MainChainServiceTests, ForkWhenPeerHasShorterChain)
{
  auto gen  = block_generator_();
  auto b1   = block_generator_(gen);
  auto b2   = block_generator_(b1);
  auto b3   = block_generator_(b2);
  auto b4   = block_generator_(b3);
  auto b5_1 = block_generator_(b4);
  auto b6_1 = block_generator_(b5_1);

  MainChain         other1_chain;
  MainChainProtocol other1_proto{other1_chain};
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b1));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b2));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b3));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b4));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b5_1));
  ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*b6_1));

  auto b5_2 = block_generator_(b4);
  auto b6_2 = block_generator_(b5_2);
  auto b7_2 = block_generator_(b6_2);

  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b1));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b2));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b3));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b4));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b5_2));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b6_2));
  ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*b7_2));

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  auto const log1 = other1_proto.TimeTravel(b6_2->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, b6_2->hash)).WillOnce(Return(CreatePromise(log1)));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  auto const log2 = other1_proto.TimeTravel(b5_2->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, b5_2->hash)).WillOnce(Return(CreatePromise(log2)));

  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  auto const log3 = other1_proto.TimeTravel(b4->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, b4->hash)).WillOnce(Return(CreatePromise(log3)));

  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  Tick(State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), b7_2->hash);

  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
}

TEST_F(MainChainServiceTests, CheckWaitingToFullfilResponse)
{
  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  auto promise = fetch::service::MakePromise();

  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(TraveloguePromise{promise}));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);

  // simulate the response taking time to arrive
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);

  // simulate a failure as that is easier
  promise->Fail();

  // trigger re-sync to same peer
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
}

TEST_F(MainChainServiceTests, CheckHandlingOfEmptyLog)
{
  // generate invalid payload from client
  MainChainProtocol::Travelogue log{};
  log.status = fetch::ledger::TravelogueStatus::HEAVIEST_BRANCH;

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(CreatePromise(log)));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);

  // upon invalid message from the peer we simply conclude our sync with them
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
}

TEST_F(MainChainServiceTests, CheckHandlingOfUnserialisablePayload)
{
  auto promise = fetch::service::MakePromise();
  promise->Fulfill(fetch::byte_array::ConstByteArray{});  // empty buffer will cause de-ser errors

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(TraveloguePromise{promise}));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);

  // upon invalid message from the peer we simply conclude our sync with them
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
}

TEST_F(MainChainServiceTests, CheckRetryMechanism)
{
  auto failed = fetch::service::MakePromise();
  failed->Fail();

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillRepeatedly(Return(TraveloguePromise{failed}));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);

  // attempt 1
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  // attempt 2
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  // attempt 3
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
}

TEST_F(MainChainServiceTests, CheckPeriodicResync)
{
  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{}));

  Tick(State::SYNCHRONISING, State::SYNCHRONISED);

  // should stay in sync'ed state
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  clock_->AddOffset(std::chrono::seconds{30});

  Tick(State::SYNCHRONISED, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);

  // should stay in sync'ed state
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
}

TEST_F(MainChainServiceTests, CheckLooseBlocksTrigger)
{
  auto gen = block_generator_();
  auto b1  = block_generator_(gen);
  auto b2  = block_generator_(b1);

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{}));

  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  // simulate the blocks being ahead of consensus prompting a resync
  EXPECT_CALL(consensus_, ValidBlock(*b2)).WillRepeatedly(Return(ConsensusInterface::Status::NO));

  // simulate the arrival of gossiped blocks
  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED, State::SYNCHRONISING);
}

TEST_F(MainChainServiceTests, CheckWhenGenesisAppearsToBeInvalid)
{
  MainChainProtocol::Travelogue log{};
  log.status = fetch::ledger::TravelogueStatus::NOT_FOUND;

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(CreatePromise(log)));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
}

}  // namespace
