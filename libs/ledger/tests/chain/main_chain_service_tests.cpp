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
#include "core/containers/vector_util.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/ecdsa.hpp"
#include "gtest/gtest.h"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "ledger/testing/block_generator.hpp"
#include "mock_consensus.hpp"
#include "mock_main_chain_rpc_client.hpp"
#include "mock_muddle_endpoint.hpp"
#include "mock_trust_system.hpp"
#include "moment/clocks.hpp"
#include "muddle/network_id.hpp"

#include <string>
#include <unordered_map>

using ::testing::InSequence;
using ::testing::MatchResultListener;
using ::testing::MatcherInterface;
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

namespace fetch {
namespace byte_array {

inline void PrintTo(ConstByteArray const &digest, std::ostream *s)
{
  *s << "" << std::string(digest.ToHex()).substr(0, 8);
}

}  // namespace byte_array
}  // namespace fetch

namespace {

class DigestMatcher : public MatcherInterface<fetch::byte_array::ConstByteArray>
{
public:
  using type     = fetch::byte_array::ConstByteArray;
  using Patterns = std::unordered_map<type, std::string>;

  explicit DigestMatcher(type expected)
    : expected_(std::move(expected))
  {}

  DigestMatcher(type expected, Patterns const &patterns)
    : expected_(std::move(expected))
    , patterns_(&patterns)
  {}

  bool MatchAndExplain(type actual, MatchResultListener *listener) const override
  {
    if (actual == expected_)
    {
      return true;
    }
    if (static_cast<bool>(patterns_))
    {
      Identify(actual, listener);
    }
    return false;
  }

  void DescribeTo(std::ostream *os) const override
  {
    *os << Show(expected_);
    if (static_cast<bool>(patterns_))
    {
      *os << ", ";
      Identify(expected_, os);
    }
  }

  template <class... NamesAndContainers>
  static Patterns MakePatterns(NamesAndContainers &&... names_and_containers)
  {
    return KeepPatterns(Patterns{}, std::forward<NamesAndContainers>(names_and_containers)...);
  }

private:
  template <template <class...> class Container, class... ContainerArgs,
            class... NamesAndContainers>
  static Patterns KeepPatterns(
      Patterns patterns, std::string name,
      Container<fetch::ledger::BlockPtr, ContainerArgs...> const &container,
      NamesAndContainers &&... names_and_containers);

  static Patterns KeepPatterns(Patterns patterns)
  {
    return patterns;
  }

  static std::string Show(type const &hash)
  {
    return std::string(hash.ToHex().SubArray(0, 8));
  }

  template <class Stream>
  void Identify(type const &hash, Stream *stream) const
  {
    auto position = patterns_->find(hash);
    if (position != patterns_->end())
    {
      *stream << "which is at " << position->second;
    }
    else
    {
      *stream << "unknown so far";
    }
  }

  type            expected_;
  Patterns const *patterns_ = nullptr;
};

template <template <class...> class Container, class... ContainerArgs, class... NamesAndContainers>
DigestMatcher::Patterns DigestMatcher::KeepPatterns(
    Patterns patterns, std::string const &name,
    Container<fetch::ledger::BlockPtr, ContainerArgs...> const &container,
    NamesAndContainers &&... names_and_containers)
{
  std::size_t index{};
  for (auto const &block : container)
  {
    patterns.emplace(block->hash, name + '[' + std::to_string(index++) + ']');
  }
  return KeepPatterns(std::move(patterns),
                      std::forward<NamesAndContainers>(names_and_containers)...);
}

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

  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), blocks[1]->hash);

  auto travelogue2 = other1_proto.TimeTravel(blocks[1]->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, blocks[1]->hash))
      .WillOnce(Return(CreatePromise(travelogue2)));

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

  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  auto const log2 = other1_proto.TimeTravel(common_root.back()->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, common_root.back()->hash))
      .WillOnce(Return(CreatePromise(log2)));

  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

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

  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  auto const log2 = other1_proto.TimeTravel(local_branch[0]->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, local_branch[0]->hash))
      .WillOnce(Return(CreatePromise(log1)));

  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  auto const log3 = other1_proto.TimeTravel(common_root[2]->hash);
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, common_root[2]->hash))
      .WillOnce(Return(CreatePromise(log3)));

  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

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

  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);

  // attempt 1
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  // attempt 2
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS, State::REQUEST_NEXT_BLOCKS);

  // attempt 3
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

  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS,
             State::WAIT_FOR_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);
}

namespace {

MainChainProtocol::Travelogue TimeTravel(BlockGenerator::BlockPtr const &heaviest_block,
                                         BlockGenerator::BlockPtrs       local_blocks)
{
  return {heaviest_block->hash, heaviest_block->block_number, TravelogueStatus::HEAVIEST_BRANCH,
          std::move(local_blocks)};
}

}  // namespace

TEST_F(MainChainServiceTests, CheckExponentialBackStep)
{
  InSequence s;
  auto       gen = block_generator_();

  static constexpr std::size_t pack_size = 10000;

  auto common_part    = block_generator_(2 * pack_size, gen);
  auto fake_branch    = block_generator_(5 * pack_size, common_part.back());
  auto genuine_branch = block_generator_(3 * pack_size, common_part.back(), 10);  // heavier

  auto        fake_heaviest    = block_generator_(fake_branch.back());
  auto const &genuine_heaviest = genuine_branch.back();

  PrintTo(fake_heaviest->hash, &(std::cerr << "Fake heaviest: "));
  std::cerr << ", " << fake_heaviest->block_number << '\n';
  PrintTo(fake_branch.back()->hash, &(std::cerr << "Fake latest: "));
  std::cerr << ", " << fake_branch.back()->block_number << '\n';

  auto known_hashes = DigestMatcher::MakePatterns("common_part", common_part, "fake_branch",
                                                  fake_branch, "genuine_branch", genuine_branch);

  auto expected_hash = [&](fetch::byte_array::ConstByteArray expected) {
    return MakeMatcher(new DigestMatcher(std::move(expected), known_hashes));
  };

  MainChainProtocol::Travelogue log;
  log.status = TravelogueStatus::HEAVIEST_BRANCH;

  EXPECT_CALL(endpoint_, GetDirectlyConnectedPeers()).WillRepeatedly(Return(AddressList{other1_}));

  // build a fake chain
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, expected_hash(GetGenesisDigest())))
      .WillOnce(Return(CreatePromise(TimeTravel(fake_heaviest, common_part))));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, expected_hash(common_part.back()->hash)))
      .WillOnce(Return(CreatePromise(TimeTravel(fake_heaviest, fake_branch))));
  // denounce this chain
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 1]->hash)))
      .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 2]->hash)))
      .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 4]->hash)))
      .WillOnce(Return(CreatePromise(MainChainProtocol::Travelogue{})));
  EXPECT_CALL(rpc_client_, TimeTravel(other1_, expected_hash(fake_branch[5 * pack_size - 8]->hash)))
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
  FollowPath(State::SYNCHRONISING, State::START_SYNC_WITH_PEER, State::REQUEST_NEXT_BLOCKS);
  // build a fake chain
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // common_part
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch

  // denounce fake chain
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 1]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 2]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 4]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 8]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 16]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 32]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 64]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 128]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 256]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 512]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 1024]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 2048]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 4096]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 8192]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 16384]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 32768]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // fake_branch[5 * pack_size - 49152]

  // now build the genuine chain
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);  // common_part[2 * pack_size - 15536]
  FollowPath(State::REQUEST_NEXT_BLOCKS, State::WAIT_FOR_NEXT_BLOCKS,
             State::REQUEST_NEXT_BLOCKS);                            // common_part.back()
  Tick(State::REQUEST_NEXT_BLOCKS, State::COMPLETE_SYNC_WITH_PEER);  // and here it ends

  EXPECT_EQ(chain_.GetHeaviestBlockHash(), genuine_heaviest->hash);
}

}  // namespace
