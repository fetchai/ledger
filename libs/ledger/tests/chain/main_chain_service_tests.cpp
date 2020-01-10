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
using fetch::ledger::TravelogueStatus;

using AddressList        = fetch::muddle::MuddleEndpoint::AddressList;
using State              = MainChainRpcService::State;
using MuddleAddress      = fetch::muddle::Address;
using TraveloguePromise  = fetch::network::PromiseOf<MainChainProtocol::Travelogue>;
using AdjustableClockPtr = fetch::moment::AdjustableClockPtr;

std::ostream &operator<<(std::ostream &s, MainChainRpcService::State state)
{
  return s << fetch::ledger::ToString(state);
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

  void Tick(State persistent_state, int line);
  void Tick(State current_state, State next_state, int line);

  template <class... States>
  void FollowPath(int line, State current, States... subsequent)
  {
    State trajectory[] = {subsequent...};

    for (auto next_state : trajectory)
    {
      Tick(current, next_state, line);
      current = next_state;
    }
  }

  AdjustableClockPtr               clock_{fetch::moment::CreateAdjustableClock("MC_RPC:main")};
  BlockGenerator                   block_generator_{NUM_LANES, NUM_SLICES};
  ECDSASigner                      self_;
  ECDSASigner                      other1_signer_;
  MuddleAddress                    other1_{other1_signer_.identity().identifier()};
  ECDSASigner                      other2_signer_;
  MuddleAddress                    other2_{other1_signer_.identity().identifier()};
  NiceMock<MockMainChainRpcClient> rpc_client_;
  NiceMock<MockMuddleEndpoint>     endpoint_{self_.identity().identifier(), NetworkId{"TEST"}};
  NiceMock<MockConsensus>          consensus_;
  NiceMock<MockTrustSystem>        trust_;
  MainChain                        chain_;
  MainChainRpcService              rpc_service_{endpoint_, rpc_client_, chain_, trust_,
                                   CreateNonOwning(consensus_)};
};

void MainChainServiceTests::Tick(State persistent_state, int line)
{
  // the machine should stay in this same state
  Tick(persistent_state, persistent_state, line);
}

namespace {

void AssertEq(char const *tick_phase, State actual, State expected, int line)
{
  ASSERT_EQ(actual, expected) << "when asserting " << tick_phase << "-tick state at line " << line
                              << ": RPC service is " << ToString(actual)
                              << " but was expected to be " << ToString(expected);
}

}  // namespace

void MainChainServiceTests::Tick(State current_state, State next_state, int line)
{
  AssertEq("pre", rpc_service_.state(), current_state, line);
  auto sm = rpc_service_.GetWeakRunnable().lock();
  sm->Execute();
  AssertEq("post", rpc_service_.state(), next_state, line);
}

#define Tick(...) Tick(__VA_ARGS__, __LINE__)
#define FollowPath(...) FollowPath(__LINE__, __VA_ARGS__)

TEST_F(MainChainServiceTests, CheckNoPeersCase)
{
  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{}));

  Tick(State::SYNCHRONISING, State::SYNCHRONISED);

  // should stay in sync'ed state
  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
}

TEST_F(MainChainServiceTests, CheckSimpleCatchUpFromSinglePeer)
{
  auto gen    = block_generator_();
  auto blocks = block_generator_(4, std::move(gen));

  MainChain         other1_chain;
  MainChainProtocol other1_proto{other1_chain};
  for (auto const &block : blocks)
  {
    EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*block))
        << " when adding block #" << block->block_number;
  }

  auto travelogue = other1_proto.TimeTravel(GetGenesisDigest());

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(CreatePromise(travelogue)));

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  Tick(State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS,
             State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), blocks.back()->hash);
}

TEST_F(MainChainServiceTests, ChecIncrementalCatchUp)
{
  auto gen    = block_generator_();
  auto blocks = block_generator_(4, std::move(gen));

  MainChain         other1_chain;
  MainChainProtocol other1_proto{other1_chain};
  for (auto const &block : blocks)
  {
    EXPECT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*block))
        << " when adding block #" << block->block_number;
  }

  auto travelogue1 = other1_proto.TimeTravel(GetGenesisDigest());
  travelogue1.blocks.resize(2);  // simulate large sync forward in time

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(CreatePromise(travelogue1)));

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), blocks[1]->hash);

  auto travelogue2 = other1_proto.TimeTravel(blocks[1]->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, blocks[1]->hash))
      .WillOnce(Return(CreatePromise(travelogue2)));

  /*
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  Tick(State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);
  */
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS,
             State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);

  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), blocks.back()->hash);
}

TEST_F(MainChainServiceTests, ForkWhenPeerHasLongerChain)
{
  MainChain         other1_chain;
  MainChainProtocol other1_proto{other1_chain};

  auto gen           = block_generator_();
  auto common_root   = block_generator_(4, gen);
  auto local_branch  = block_generator_(2, common_root.back());
  auto remote_branch = block_generator_(3, common_root.back());

  for (auto const &block : common_root)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*block))
        << " when adding block #" << block->block_number;
    ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*block))
        << " when adding block #" << block->block_number;
  }

  for (auto const &block : local_branch)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*block))
        << " when adding block #" << block->block_number;
  }

  for (auto const &block : remote_branch)
  {
    ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*block))
        << " when adding block #" << block->block_number;
  }

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  auto const log1 = other1_proto.TimeTravel(local_branch.front()->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, local_branch.front()->hash))
      .WillOnce(Return(CreatePromise(log1)));

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  auto const log2 = other1_proto.TimeTravel(common_root.back()->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, common_root.back()->hash))
      .WillOnce(Return(CreatePromise(log2)));

  /*
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  /*
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  Tick(State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);
  */
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), remote_branch.back()->hash);

  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
}

TEST_F(MainChainServiceTests, ForkWhenPeerHasShorterChain)
{
  MainChain         other1_chain;
  MainChainProtocol other1_proto{other1_chain};

  auto gen           = block_generator_();
  auto common_root   = block_generator_(4, gen);
  auto remote_branch = block_generator_(2, common_root.back());
  auto local_branch  = block_generator_(3, common_root.back());

  for (auto const &block : common_root)
  {
    ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*block))
        << " when adding block #" << block->block_number;
    ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*block))
        << " when adding block #" << block->block_number;
  }

  for (auto const &block : remote_branch)
  {
    ASSERT_EQ(BlockStatus::ADDED, other1_chain.AddBlock(*block))
        << " when adding block #" << block->block_number;
  }

  for (auto const &block : local_branch)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_.AddBlock(*block))
        << " when adding block #" << block->block_number;
  }

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  auto const log1 = other1_proto.TimeTravel(local_branch[1]->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, local_branch[1]->hash))
      .WillOnce(Return(CreatePromise(log1)));

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  auto const log2 = other1_proto.TimeTravel(local_branch[0]->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, local_branch[0]->hash))
      .WillOnce(Return(CreatePromise(log1)));

  /*
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  auto const log3 = other1_proto.TimeTravel(common_root[2]->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, common_root[2]->hash))
      .WillOnce(Return(CreatePromise(log3)));

  /*
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  /*
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  Tick(State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);
  */
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), local_branch.back()->hash);

  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
}

TEST_F(MainChainServiceTests, CheckWaitingToFullfilResponse)
{
  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  auto promise = fetch::service::MakePromise();

  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(TraveloguePromise{promise}));

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS);

  // simulate the response taking time to arrive
  Tick(State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS);

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

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS);

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

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS);

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

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);

  // attempt 1
  /*
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  // attempt 2
  /*
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  // attempt 3
  /*
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  */
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
}

TEST_F(MainChainServiceTests, CheckPeriodicResync)
{
  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{}));

  Tick(State::SYNCHRONISING, State::SYNCHRONISED);

  // should stay in sync'ed state
  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);

  clock_->AddOffset(std::chrono::seconds{30});

  /*
  Tick(State::SYNCHRONISED, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  */
  FollowPath(State::SYNCHRONISED, State::SYNCHRONISING, State::SYNCHRONISED);

  // should stay in sync'ed state
  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);
}

TEST_F(MainChainServiceTests, CheckLooseBlocksTrigger)
{
  auto gen = block_generator_();
  auto b1  = block_generator_(gen);
  auto b2  = block_generator_(b1);

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{}));

  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED);

  // simulate the blocks being ahead of consensus prompting a resync
  EXPECT_CALL(consensus_, ValidBlock(*b2)).WillRepeatedly(Return(ConsensusInterface::Status::NO));

  // simulate the arrival of gossiped blocks
  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED);

  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED);

  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED);

  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED);

  rpc_service_.OnNewBlock(other1_, *b2, other1_);
  Tick(State::SYNCHRONISED);

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

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
}

namespace {

MainChainProtocol::Travelogue TimeTravel(BlockGenerator::BlockPtr const & heaviest_block,
                                         BlockGenerator::BlockPtrs const &local_blocks)
{
  MainChainProtocol::Travelogue::Blocks blocks;
  blocks.reserve(local_blocks.size());
  for (auto const &block : local_blocks)
  {
    blocks.push_back(*block);
  }

  return {heaviest_block->hash, heaviest_block->block_number, TravelogueStatus::HEAVIEST_BRANCH,
          std::move(blocks)};
}

}  // namespace

TEST_F(MainChainServiceTests, CheckExponentialBackStep)
{
  auto gen = block_generator_();

  static constexpr std::size_t pack_size = 10000;

  auto first_pack  = block_generator_(pack_size, gen);
  auto second_pack = block_generator_(pack_size, first_pack.back());

  auto wrong_third_pack  = block_generator_(pack_size, second_pack.back());
  auto wrong_fourth_pack = block_generator_(pack_size, wrong_third_pack.back());
  auto wrong_fifth_pack  = block_generator_(pack_size, wrong_fourth_pack.back());
  auto wrong_heaviest    = block_generator_(wrong_fifth_pack.back());

  auto right_third_pack  = block_generator_(pack_size - 6384, second_pack.back());
  auto right_fourth_pack = block_generator_(pack_size, right_third_pack.back());
  auto right_fifth_pack  = block_generator_(pack_size, right_fourth_pack.back(), 10);  // heavier
  // auto const &right_heaviest = right_fifth_pack.back();

  MainChainProtocol::Travelogue log;
  log.status = TravelogueStatus::HEAVIEST_BRANCH;

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  // build wrong chain
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, GetGenesisDigest()))
      .WillOnce(Return(CreatePromise(TimeTravel(wrong_heaviest, first_pack))));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, first_pack.back()->hash))
      .WillOnce(Return(CreatePromise(TimeTravel(wrong_heaviest, second_pack))));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, second_pack.back()->hash))
      .WillOnce(Return(CreatePromise(TimeTravel(wrong_heaviest, wrong_third_pack))));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_third_pack.back()->hash))
      .WillOnce(Return(CreatePromise(TimeTravel(wrong_heaviest, wrong_fourth_pack))));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fourth_pack.back()->hash))
      .WillOnce(Return(CreatePromise(TimeTravel(wrong_heaviest, wrong_fifth_pack))));
  // denounce this chain
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack.back()->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 1]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 2]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 4]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 8]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 16]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 32]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 64]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 128]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 256]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 512]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 1024]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 2048]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 4096]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fifth_pack[pack_size - 8192]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_fourth_pack[pack_size - 6384]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, wrong_third_pack[pack_size - 6384]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));

  BlockGenerator::BlockPtrs right_pack(second_pack.cend() - 6384, second_pack.cend());

  EXPECT_CALL(rpc_client_, TimeTravel(other1_, second_pack[pack_size - 6384]->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::TimeTravelogue{})));

  /*
  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  */
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);  // first_pack
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);  // second_pack
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);  // wrong_third_pack
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);  // wrong_fourth_pack
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);  // wrong_fifth_pack

  /*

  EXPECT_CALL(rpc_client_, TimeTravel(other1_, second_pack.back()->hash))
      .WillOnge(Return(CreatePromise(TimeTravel(wrong_heaviest, wrong_third_pack))));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, third_pack.back()->hash))
      .WillOnce(Return(CreatePromise(TimeTravel(wrong_heaviest, wrong_fourth_pack))));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, fourth_pack.back()->hash))
      .WillOnce(Return(CreatePromise(TimeTravel(wrong_heaviest, wrong_fifth_pack))));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, fifth_pack.back()->hash))
      .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{right_heaviest->hash,
  right_heaviest->block_number})));

  Tick(State::SYNCHRONISING, State::START_SYNC_WITH_PEER);
  Tick(State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS);
  Tick(State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
  Tick(State::COMPLETE_SYNC_WITH_PEER, State::SYNCHRONISED);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), blocks.back()->hash);
  */
}

}  // namespace
