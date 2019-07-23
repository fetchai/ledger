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

#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "dkg/dkg.hpp"
#include "dkg/rbc.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

#include "gtest/gtest.h"
#include <iostream>

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
using ConstByteArray = fetch::byte_array::ConstByteArray;

ProverPtr CreateNewCertificate()
{
  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate = std::make_shared<Signer>();

  certificate->GenerateKeys();

  return certificate;
}

uint16_t CHANNEL_SHARES = 3;

struct CabinetMember
{
  uint16_t                              muddle_port;
  NetworkManager                        network_manager;
  ProverPtr                             muddle_certificate;
  Muddle                                muddle;
  std::shared_ptr<muddle::Subscription> shares_subscription;
  RBC                                   rbc;
  DistributedKeyGeneration              dkg;

  // Set when DKG is finished
  bn::Fr              secret_share;
  bn::G2              public_key;
  RBC::CabinetMembers qual_set;
  std::vector<bn::G2> public_key_shares;

  CabinetMember(uint16_t port_number, uint16_t index, RBC::CabinetMembers const &current_cabinet,
                uint32_t const &threshold)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 3}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate, network_manager, true,
             true}
    , shares_subscription(muddle.AsEndpoint().Subscribe(SERVICE_DKG, CHANNEL_SHARES))
    , rbc{muddle.AsEndpoint(), muddle_certificate->identity().identifier(), current_cabinet,
          [this](ConstByteArray const &address, ConstByteArray const &payload) -> void {
            DKGEnvelop    env;
            DKGSerializer serializer{payload};
            serializer >> env;
            dkg.OnDkgMessage(address, env.Message());
          }}
    , dkg{muddle_certificate->identity().identifier(), current_cabinet, threshold,
          [this](DKGEnvelop const &envelop) -> void {
            DKGSerializer serialiser;
            envelop.Serialize(serialiser);
            rbc.SendRBroadcast(serialiser.data());
          },
          [this](ConstByteArray const &                     destination,
                 std::pair<std::string, std::string> const &shares) -> void {
            SubmitShare(destination, shares);
          }}
  {
    // Set subscription for rbc
    shares_subscription->SetMessageHandler([this](ConstByteArray const &from, uint16_t, uint16_t,
                                                  uint16_t, muddle::Packet::Payload const &payload,
                                                  ConstByteArray) {
      fetch::serializers::ByteArrayBuffer serialiser(payload);

      // Deserialize the RBCEnvelop
      std::pair<std::string, std::string> shares;
      serialiser >> shares;

      // Dispatch the event
      dkg.OnNewShares(from, shares);
    });

    network_manager.Start();
    muddle.Start({muddle_port});
  }

  void SubmitShare(ConstByteArray const &                     destination,
                   std::pair<std::string, std::string> const &shares)
  {
    fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> counter;
    counter << shares;

    fetch::serializers::ByteArrayBuffer serializer;
    serializer.Reserve(counter.size());
    serializer << shares;
    muddle.AsEndpoint().Send(destination, SERVICE_DKG, CHANNEL_SHARES, serializer.data());
  }
  void SetOutput()
  {
    dkg.SetDkgOutput(public_key, secret_share, public_key_shares, qual_set);
  }
};

TEST(dkg, small_scale_test)
{

  uint32_t            cabinet_size = 10;
  RBC::CabinetMembers cabinet;
  uint32_t            threshold{3};

  std::vector<std::unique_ptr<CabinetMember>> committee;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    committee.emplace_back(new CabinetMember{port_number, ii, cabinet, threshold});
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

  // Make sure everyone is connected to everyone else
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

  for (auto &member : committee)
  {
    cabinet.insert(member->muddle_certificate->identity().identifier());
  }
  assert(cabinet.size() == cabinet_size);

  // Reset cabinet
  for (auto &member : committee)
  {
    member->dkg.ResetCabinet();
    member->rbc.ResetCabinet();
  }

  // Start at DKG
  {
    for (auto &member : committee)
    {
      member->dkg.BroadcastShares();
    }

    // Loop until everyone is finished with DKG
    uint32_t kk = 0;
    while (kk != cabinet_size)
    {
      std::this_thread::sleep_for(std::chrono::seconds(30));
      for (uint32_t mm = kk; mm < cabinet_size; ++mm)
      {
        if (!committee[mm]->dkg.finished())
        {
          break;
        }
        else
        {
          ++kk;
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Set DKG outputs
    for (auto &member : committee)
    {
      member->SetOutput();
    }
    // Check DKG is working correctly
    for (uint32_t nn = 1; nn < cabinet_size; ++nn)
    {
      EXPECT_EQ(committee[0]->public_key, committee[nn]->public_key);
      EXPECT_EQ(committee[0]->public_key_shares, committee[nn]->public_key_shares);
      EXPECT_NE(committee[0]->public_key_shares[0], committee[nn]->public_key_shares[nn]);
      for (uint32_t pp = nn + 1; pp < cabinet_size; ++pp)
      {
        EXPECT_NE(committee[0]->public_key_shares[nn], committee[0]->public_key_shares[pp]);
      }
    }
  }

  for (auto &member : committee)
  {
    member->muddle.Stop();
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
}
