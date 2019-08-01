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
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "dkg/dkg_service.hpp"
#include "network/muddle/muddle.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using namespace fetch::network;
using namespace fetch::crypto;
using namespace fetch::muddle;
using namespace fetch::dkg;

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

int main()
{

  uint32_t cabinet_size = 4;

  struct CabinetMember
  {
    uint16_t             muddle_port;
    NetworkManager       network_manager;
    fetch::core::Reactor reactor;
    ProverPtr            muddle_certificate;
    Muddle               muddle;
    DkgService           dkg_service;
    CabinetMember(uint16_t port_number, uint16_t index)
      : muddle_port{port_number}
      , network_manager{"NetworkManager" + std::to_string(index), 1}
      , reactor{"ReactorName" + std::to_string(index)}
      , muddle_certificate{CreateNewCertificate()}
      , muddle{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate, network_manager, true,
               true}
      , dkg_service{muddle.AsEndpoint(), muddle_certificate->identity().identifier()}
    {
      network_manager.Start();
      muddle.Start({muddle_port});
    }
  };

  std::vector<std::unique_ptr<CabinetMember>> committee;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    committee.emplace_back(new CabinetMember{port_number, ii});
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Connect muddles together (localhost for this example)
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    for (uint32_t jj = ii + 1; jj < cabinet_size; jj++)
    {
      committee[ii]->muddle.AddPeer(
          fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(committee[jj]->muddle_port)});
    }
  }

  uint32_t kk = 0;
  while (kk != cabinet_size)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (uint32_t mm = kk; mm < cabinet_size; ++mm)
    {
      if (committee[mm]->muddle.AsEndpoint().GetDirectlyConnectedPeers().size() !=
          (cabinet_size - 1))
      {
        break;
      }
      else
      {
        ++kk;
      }
    }
  }

  RBC::CabinetMembers cabinet;
  for (auto &member : committee)
  {
    cabinet.insert(member->muddle_certificate->identity().identifier());
  }
  assert(cabinet.size() == cabinet_size);

  // Start at RBC for each muddle
  {
    uint32_t threshold{1};

    for (auto &member : committee)
    {
      member->dkg_service.ResetCabinet(cabinet, threshold);
      member->reactor.Attach(member->dkg_service.GetWeakRunnable());
    }

    // Start DKG
    for (auto &member : committee)
    {
      member->reactor.Start();
    }

    // Need to increase this depending on number of nodes to complete 3 rounds of DRB
    std::this_thread::sleep_for(std::chrono::seconds(25));
  }

  for (auto &member : committee)
  {
    member->reactor.Stop();
    member->muddle.Stop();
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));

  return 0;
}
