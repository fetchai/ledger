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
#include "core/reactor.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"

#include "network/generics/requesting_queue.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"

#include "beacon/beacon_service.hpp"
#include "beacon/beacon_setup_protocol.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/cabinet_member_details.hpp"
#include "beacon/entropy.hpp"
#include "beacon/event_manager.hpp"

#include <cstdint>
#include <deque>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using namespace fetch;
using namespace fetch::beacon;

#include "gtest/gtest.h"
#include <iostream>

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using Address        = fetch::muddle::Packet::Address;

ProverPtr CreateNewCertificate()
{
  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate = std::make_shared<Signer>();

  certificate->GenerateKeys();

  return certificate;
}

struct CabinetNode
{
  using Prover         = crypto::Prover;
  using ProverPtr      = std::shared_ptr<Prover>;
  using Certificate    = crypto::Prover;
  using CertificatePtr = std::shared_ptr<Certificate>;
  using Muddle         = muddle::MuddlePtr;

  EventManager::SharedEventManager event_manager;
  uint16_t                         muddle_port;
  network::NetworkManager          network_manager;
  core::Reactor                    reactor;
  ProverPtr                        muddle_certificate;
  Muddle                           muddle;
  BeaconService                    beacon_service;
  crypto::Identity                 identity;

  CabinetNode(uint16_t port_number, uint16_t index)
    : event_manager{EventManager::New()}
    , muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{muddle::CreateMuddle("Test", muddle_certificate, network_manager, "127.0.0.1")}
    , beacon_service{muddle->GetEndpoint(), muddle_certificate, event_manager}
    , identity{muddle_certificate->identity()}
  {
    network_manager.Start();
    muddle->Start({}, {muddle_port});
  }

  muddle::Address GetMuddleAddress() const
  {
    return muddle->GetAddress();
  }

  network::Uri GetHint() const
  {
    return fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(muddle_port)};
  }
};

void RunHonestComitteeRenewal(uint16_t delay = 100, uint16_t total_renewals = 4,
                              uint16_t number_of_cabinets = 4, uint16_t cabinet_size = 4,
                              uint16_t numbers_per_aeon = 10, double threshold = 0.5)
{
  std::cout << "- Setup" << std::endl;
  uint16_t number_of_nodes = static_cast<uint16_t>(number_of_cabinets * cabinet_size);
  // Initialising the BLS library
  crypto::bls::Init();

  std::vector<std::unique_ptr<CabinetNode>> committee;
  for (uint16_t ii = 0; ii < number_of_nodes; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    committee.emplace_back(new CabinetNode{port_number, ii});
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Connect muddles together (localhost for this example)
  for (uint32_t ii = 0; ii < number_of_nodes; ii++)
  {
    for (uint32_t jj = ii + 1; jj < number_of_nodes; jj++)
    {
      committee[ii]->muddle->ConnectTo(committee[jj]->GetMuddleAddress(), committee[jj]->GetHint());
    }
  }

  // Creating n cabinets
  std::vector<BeaconService::CabinetMemberList> all_cabinets;
  all_cabinets.resize(number_of_cabinets);

  uint64_t i = 0;
  for (auto &member : committee)
  {
    all_cabinets[i % number_of_cabinets].insert(member->muddle_certificate->identity());
    ++i;
  }

  // Attaching the cabinet logic
  for (auto &member : committee)
  {
    member->reactor.Attach(member->beacon_service.GetMainRunnable());
    member->reactor.Attach(member->beacon_service.GetSetupRunnable());
  }

  // Starting the beacon
  for (auto &member : committee)
  {
    member->reactor.Start();
  }

  // Stats
  std::unordered_map<crypto::Identity, uint64_t> rounds_finished;
  for (auto &member : committee)
  {
    rounds_finished[member->identity] = 0;
  }

  // Ready
  i = 0;
  while (i < static_cast<uint64_t>(total_renewals + 1))
  {
    auto cabinet = all_cabinets[i % number_of_cabinets];

    if (i < total_renewals)
    {
      std::cout << "- Scheduling round " << i << std::endl;
      for (auto &member : committee)
      {
        member->beacon_service.StartNewCabinet(
            cabinet, static_cast<uint32_t>(static_cast<double>(cabinet.size()) * threshold),
            i * numbers_per_aeon, (i + 1) * numbers_per_aeon);
      }
    }

    // Collecting information about the committees finishing
    for (uint64_t j = 0; j < 30; ++j)
    {

      for (auto &member : committee)
      {
        // Polling events about aeons completed work
        fetch::beacon::EventCommitteeCompletedWork event;
        while (member->event_manager->Poll(event))
        {
          ++rounds_finished[member->identity];
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    ++i;
  }

  // Stopping all
  std::cout << " - Waiting" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << " - Stopping" << std::endl;
  for (auto &member : committee)
  {
    member->reactor.Stop();
    member->network_manager.Stop();
  }

  std::cout << " - Testing" << std::endl;
  // Verifying stats
  // TODO(tfr): Check that the hashes are acutally the same
  for (auto finish_stat : rounds_finished)
  {
    EXPECT_EQ(finish_stat.second, total_renewals);
  }
}

TEST(beacon, full_cycle)
{
  SetGlobalLogLevel(LogLevel::CRITICAL);
  // TODO(tfr): Heuristically fails atm. RunHonestComitteeRenewal(100, 4, 4, 4, 10, 0.5);
  RunHonestComitteeRenewal(400, 4, 2, 2, 10, 0.5);
}
