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

#include "block_storage_tool.hpp"
#include "wait_for_connections.hpp"

#include "network/p2pservice/p2ptrust_interface.hpp"

#include <chrono>
#include <thread>

namespace {

using namespace std::chrono_literals;

using fetch::muddle::CreateMuddle;

using MuddleAddress = fetch::muddle::Address;

constexpr char const *LOGGING_NAME = "BlockStoreTool";

class DummyTrust : public fetch::p2p::P2PTrustInterface<MuddleAddress>
{
public:
  using TrustSubject = fetch::p2p::TrustSubject;
  using TrustQuality = fetch::p2p::TrustQuality;

  // clang-format off
  void        AddFeedback(MuddleAddress const &, TrustSubject, TrustQuality) override {}
  void        AddFeedback(MuddleAddress const &, ConstByteArray const &, TrustSubject,
                          TrustQuality) override {}
  IdentitySet GetBestPeers(std::size_t) const override { return {}; }
  PeerTrusts  GetPeersAndTrusts() const override { return {}; }
  IdentitySet GetRandomPeers(std::size_t, double) const override { return {}; }
  std::size_t GetRankOfPeer(MuddleAddress const &) const override {return 0;}
  double      GetTrustRatingOfPeer(MuddleAddress const &) const override { return 0.0;}
  bool        IsPeerTrusted(MuddleAddress const &) const override {return false;}
  bool        IsPeerKnown(MuddleAddress const &) const override {return false;}
  void        Debug() const override {}
  // clang-format on
};

DummyTrust dummy_trust;

} // namespace

BlockStorageTool::BlockStorageTool()
{
  // start the network manager
  nm_.Start();

  // create the shard muddle muddle
  net_ = CreateMuddle("IHUB", nm_, "127.0.0.1");

  // start the network
  net_->Start({"tcp://127.0.0.1:8001"}, {0});

  // wait until we have established the connection to the remote peer
  if (!WaitForPeerConnections(*net_, 1))
  {
    throw std::runtime_error{"Unable to connect to peers requested"};
  }

  reactor_.Start();

//  // create the storage client
//  auto const peers = net_->GetDirectlyConnectedPeers();
//  client_          = std::make_unique<TxStorageClient>(
//      TxStorageClient::LaneAddresses{peers.begin(), peers.end()}, net_->GetEndpoint());

  FETCH_LOG_INFO(LOGGING_NAME, "Initialisation complete");
}

BlockStorageTool::~BlockStorageTool()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Tearing Down");



  // tear down
  reactor_.Stop();
  net_->Stop();
  nm_.Stop();
}

int BlockStorageTool::Run()
{
  // sync with the local database
  Sync();

  auto heaviest = chain_.GetHeaviestBlock();
  if (!heaviest)
  {
    return EXIT_FAILURE;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Heaviest Block: 0x", heaviest->hash.ToHex(), " (",
                 heaviest->block_number, ")");

  BroadcastLatest();

  // drop the sync
  FETCH_LOG_INFO(LOGGING_NAME, "TICK TICK BOOM");
  std::this_thread::sleep_for(2500ms);

  return EXIT_SUCCESS;
}

void BlockStorageTool::Sync()
{
  // create the main chain service and attach it to our reactor
  service_ = std::make_shared<MainChainRpcService>(net_->GetEndpoint(), chain_, dummy_trust,
                                                   MainChainRpcService::Mode::PRIVATE_NETWORK);
  reactor_.Attach(service_->GetWeakRunnable());

  // wait until we have sync'ed
  FETCH_LOG_INFO(LOGGING_NAME, "Waiting for chain to sync...");
  while (!service_->IsSynced())
  {
    std::this_thread::sleep_for(500ms);
  }
  FETCH_LOG_INFO(LOGGING_NAME, "Waiting for chain to sync...complete");

  // drop the sync
  service_.reset();
}

void BlockStorageTool::BroadcastLatest()
{
  service_ = std::make_shared<MainChainRpcService>(net_->GetEndpoint(), chain_, dummy_trust,
                                                   MainChainRpcService::Mode::PRIVATE_NETWORK);

  auto heaviest = chain_.GetHeaviestBlock();
  if (!heaviest)
  {
    return;
  }

  for (std::size_t i = 0; i < 10; ++i)
  {
    service_->BroadcastBlock(*heaviest);
  }

  service_.reset();
}