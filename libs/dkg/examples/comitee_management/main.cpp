#include "core/reactor.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "dkg/beacon_manager.hpp"
#include "dkg/dkg_service.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/subscription.hpp"

#include "beacon_round.hpp"
#include "beacon_service.hpp"
#include "beacon_setup_protocol.hpp"
#include "beacon_setup_service.hpp"
#include "cabinet_member_details.hpp"
#include "entropy.hpp"

#include <cstdint>
#include <deque>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using namespace fetch;
using namespace fetch::beacon;

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
  using Muddle         = muddle::Muddle;

  uint16_t                muddle_port;
  network::NetworkManager network_manager;
  core::Reactor           reactor;
  ProverPtr               muddle_certificate;
  Muddle                  muddle;
  BeaconService           beacon_service;

  CabinetNode(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate, network_manager, true,
             true}
    , beacon_service{muddle.AsEndpoint(), muddle_certificate}
  {
    network_manager.Start();
    muddle.Start({muddle_port});
  }
};

int main()
{
  constexpr uint16_t cabinet_size       = 8;
  constexpr uint16_t number_of_cabinets = 1;

  // Initialising the BLS library
  crypto::bls::Init();

  CabinetNode x{8080, 0};

  std::vector<std::unique_ptr<CabinetNode>> committee;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    committee.emplace_back(new CabinetNode{port_number, ii});
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

  // Waiting until all are connected

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

  // Creating two cabinets
  BeaconService::CabinetMemberList all_cabinets[number_of_cabinets];

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

  // Ready
  i = 0;
  while (true)
  {
    for (auto &member : committee)
    {
      auto cabinet = all_cabinets[i % number_of_cabinets];
      member->beacon_service.StartNewCabinet(cabinet, static_cast<uint32_t>(cabinet.size() / 2));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200000));
    ++i;
  }

  return 0;
}