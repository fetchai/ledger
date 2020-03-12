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

#include "digest_matcher.hpp"
#include "mock_consensus.hpp"
#include "mock_main_chain_rpc_client.hpp"
#include "mock_muddle_endpoint.hpp"
#include "mock_trust_system.hpp"

#include "chain/address.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "ledger/testing/block_generator.hpp"
#include "moment/clocks.hpp"
#include "muddle/network_id.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/registry.hpp"

#include "gtest/gtest.h"

#include <string>
#include <unordered_map>

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

using fetch::chain::GetGenesisDigest;
using fetch::crypto::ECDSASigner;
using fetch::ledger::ConsensusInterface;
using fetch::ledger::MainChain;
using fetch::ledger::MainChainProtocol;
using fetch::ledger::MainChainRpcService;
using fetch::ledger::TravelogueStatus;
using fetch::ledger::testing::BlockGenerator;
using fetch::ledger::testing::DigestMatcher;
using fetch::ledger::testing::ExpectedHash;
using fetch::muddle::NetworkId;
using fetch::serializers::LargeObjectSerializeHelper;

using AddressList        = fetch::muddle::MuddleEndpoint::AddressList;
using State              = MainChainRpcService::State;
using MuddleAddress      = fetch::muddle::Address;
using TraveloguePromise  = fetch::network::PromiseOf<MainChainProtocol::Travelogue>;
using AdjustableClockPtr = fetch::moment::AdjustableClockPtr;

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

class MainChainSyncTest : public ::testing::Test
{
protected:
  static void SetUpTestCase()
  {
    fetch::crypto::mcl::details::MCLInitialiser();
    fetch::chain::InitialiseTestConstants();
  }

  void SetUp() override
  {
    rpc_service_ = std::make_shared<MainChainRpcService>(endpoint_, rpc_client_, chain_, trust_,
                                                         CreateNonOwning(consensus_));
  }

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

  AdjustableClockPtr                   clock_{fetch::moment::CreateAdjustableClock("MC_RPC:main")};
  BlockGenerator                       block_generator_{NUM_LANES, NUM_SLICES};
  ECDSASigner                          self_;
  ECDSASigner                          other1_signer_;
  MuddleAddress                        other1_{other1_signer_.identity().identifier()};
  ECDSASigner                          other2_signer_;
  MuddleAddress                        other2_{other1_signer_.identity().identifier()};
  NiceMock<MockMainChainRpcClient>     rpc_client_;
  NiceMock<MockMuddleEndpoint>         endpoint_{self_.identity().identifier(), NetworkId{"TEST"}};
  NiceMock<MockConsensus>              consensus_;
  NiceMock<MockTrustSystem>            trust_;
  MainChain                            chain_;
  std::shared_ptr<MainChainRpcService> rpc_service_;
};

namespace {

void AssertEq(char const *tick_phase, State actual, State expected, int line)
{
  ASSERT_EQ(actual, expected) << "when asserting " << tick_phase << "-tick state at line " << line
                              << ": RPC service is " << ToString(actual)
                              << " but was expected to be " << ToString(expected);
}

MainChainProtocol::Travelogue TimeTravel(BlockGenerator::BlockPtr const &heaviest_block,
                                         BlockGenerator::BlockPtrs       local_blocks)
{
  return {heaviest_block->hash, heaviest_block->block_number, TravelogueStatus::HEAVIEST_BRANCH,
          std::move(local_blocks)};
}

}  // namespace

void MainChainSyncTest::Tick(State current_state, State next_state, int line)
{
  AssertEq("pre", rpc_service_->state(), current_state, line);
  auto sm = rpc_service_->GetWeakRunnable().lock();
  sm->Execute();
  AssertEq("post", rpc_service_->state(), next_state, line);
}

#define Tick(...) Tick(__VA_ARGS__, __LINE__)
#define FollowPath(...) FollowPath(__LINE__, __VA_ARGS__)

TEST_F(MainChainSyncTest, CheckExponentialBackStep)
{
  auto gen = block_generator_();

  static constexpr std::size_t pack_size = 10000;

  auto common_part = block_generator_(2 * pack_size, gen);

  auto fake_branch   = block_generator_(5 * pack_size, common_part.back());
  auto fake_heaviest = block_generator_(fake_branch.back());

  auto        genuine_branch = block_generator_(3 * pack_size, common_part.back(), 10);  // heavier
  auto const &genuine_heaviest = genuine_branch.back();
  FETCH_UNUSED(genuine_heaviest);

  auto known_hashes = DigestMatcher::MakePatterns("common_part", common_part, "fake_branch",
                                                  fake_branch, "genuine_branch", genuine_branch);

  auto expected_hash = [&](fetch::byte_array::ConstByteArray expected) {
    return MakeMatcher(new DigestMatcher(std::move(expected), known_hashes));
  };

  MainChainProtocol::Travelogue log;
  log.status = TravelogueStatus::HEAVIEST_BRANCH;

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));
  EXPECT_CALL(consensus_, ValidBlock(_)).WillRepeatedly(Return(ConsensusInterface::Status::YES));

  {
    InSequence s;

    // build a fake chain
    EXPECT_CALL(rpc_client_, TimeTravel(other1_, ExpectedHash(GetGenesisDigest())))
        .WillOnce(Return(CreatePromise(TimeTravel(fake_heaviest, common_part))));
    EXPECT_CALL(rpc_client_, TimeTravel(other1_, expected_hash(common_part.back()->hash)))
        .WillOnce(Return(CreatePromise(TimeTravel(fake_heaviest, fake_branch))));

    // denounce this chain
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 1]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 2]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 4]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 8]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 16]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 32]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 64]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 128]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 256]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 512]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 1024]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 2048]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 4096]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 8192]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 16384]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    // After this point, the backstride is fixed 16384 blocks.
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 32768]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 49152]->hash)))
        .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
    using BlockPtrs = BlockGenerator::BlockPtrs;
    // Finally reached the common part that is also inside the genuine chain.
    EXPECT_CALL(rpc_client_,
                TimeTravel(other1_, expected_hash(common_part[2 * pack_size - 15536]->hash)))
        .WillOnce(Return(CreatePromise(TimeTravel(
            genuine_heaviest, BlockPtrs(common_part.cend() - 15536, common_part.cend())))));
    // Ok, now return the genuine branch.
    EXPECT_CALL(rpc_client_, TimeTravel(other1_, expected_hash(common_part.back()->hash)))
        .WillOnce(Return(CreatePromise(TimeTravel(genuine_heaviest, genuine_branch))));

    // build a fake chain
    FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
               State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);  // common_part

    // denounce fake chain
    FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-1]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-2]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-4]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-8]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-16]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-32]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-64]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-128]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-256]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-512]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-1024]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-2048]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-4096]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-8192]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-16384]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-32768]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,  // fake_branch[-49152]
               // now build the genuine chain
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,      // common_part[-15536]
               State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,      // common_part.back()
               State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);  // and here it ends

    EXPECT_EQ(chain_.GetHeaviestBlockHash(), genuine_heaviest->hash);
  }
}

TEST_F(MainChainSyncTest, GenesisMismatch)
{
  MainChain::Travelogue rejected{{}, {}, TravelogueStatus::NOT_FOUND};

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, ExpectedHash(GetGenesisDigest())))
      .WillOnce(Return(CreatePromise(rejected)));

  auto counter =
      fetch::telemetry::Registry::Instance().LookupMeasurement<fetch::telemetry::Counter>(
          "ledger_mainchain_service_network_mismatches_total");
  ASSERT_TRUE(counter);
  ASSERT_EQ(counter->count(), 0);
  // counter->increment();

  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS,
             // dropped just past the opening credits
             State::COMPLETE_SYNC_WITH_PEER);

  ASSERT_EQ(counter->count(), 1);
}

}  // namespace
