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
#include "dkg/pre_dkg_sync.hpp"
#include "network/muddle/muddle.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using namespace fetch;
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
  uint32_t cabinet_size = 30;
  uint32_t threshold{16};

  struct CabinetMember
  {
    uint16_t             muddle_port;
    NetworkManager       network_manager;
    fetch::core::Reactor reactor;
    ProverPtr            muddle_certificate;
    Muddle               muddle;
    DkgService           dkg_service;
    PreDkgSync           pre_sync;
    CabinetMember(uint16_t port_number, uint16_t index)
      : muddle_port{port_number}
      , network_manager{"NetworkManager" + std::to_string(index), 1}
      , reactor{"ReactorName" + std::to_string(index)}
      , muddle_certificate{CreateNewCertificate()}
      , muddle{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate, network_manager}
      , dkg_service{muddle.AsEndpoint(), muddle_certificate->identity().identifier()}
      , pre_sync{muddle, 4}
    {
      network_manager.Start();
      muddle.Start({muddle_port});
    }
  };

  std::vector<std::unique_ptr<CabinetMember>>                         committee;
  std::unordered_map<byte_array::ConstByteArray, fetch::network::Uri> peers_list;
  RBC::CabinetMembers cabinet;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    committee.emplace_back(new CabinetMember{port_number, ii});
    peers_list.insert({committee[ii]->muddle_certificate->identity().identifier(),
                       fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(port_number)}});
    cabinet.insert(committee[ii]->muddle_certificate->identity().identifier());
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // Reset cabinet for rbc in pre-dkg sync
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    committee[ii]->pre_sync.ResetCabinet(peers_list);
    committee[ii]->dkg_service.ResetCabinet(cabinet, threshold);
  }

  // Wait until everyone else has connected
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    committee[ii]->pre_sync.Connect();
  }

  uint32_t kk = 0;
  while (kk != cabinet_size)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (uint32_t mm = kk; mm < cabinet_size; ++mm)
    {
      if (!committee[mm]->pre_sync.ready())
      {
        break;
      }
      else
      {
        ++kk;
      }
    }
  }

  // Start at DKG for each muddle
  {
    for (auto &member : committee)
    {
      member->reactor.Attach(member->dkg_service.GetWeakRunnable());
    }

    // Start DKG
    for (auto &member : committee)
    {
      member->reactor.Start();
    }

    // Sleep until everyone has finished
    uint32_t qq = 0;
    while (qq != cabinet_size)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      for (uint32_t mm = qq; mm < cabinet_size; ++mm)
      {
        if (!committee[mm]->dkg_service.IsSynced())
        {
          break;
        }
        else
        {
          ++qq;
        }
      }
    }
  }

  for (auto &member : committee)
  {
    member->reactor.Stop();
    member->muddle.Stop();
    member->muddle.Shutdown();
    member->network_manager.Stop();
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));

  return 0;
}
