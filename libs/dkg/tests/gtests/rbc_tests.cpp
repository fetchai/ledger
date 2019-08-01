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

#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
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

class FaultyRbc : public RBC
{
public:
    enum class Failures : uint8_t
    {

    };
    FaultyRbc(Endpoint &endpoint, MuddleAddress address, CabinetMembers const &cabinet,
              std::function<void(ConstByteArray const &address, ConstByteArray const &payload)> broadcast_callback,
    const std::vector<Failures> &failures = {})
    : RBC{endpoint, std::move(address), cabinet, std::move(broadcast_callback)}
    {
        for (auto f : failures)
        {
            failures_flags_.set(static_cast<uint32_t>(f));
        }
    }

private:
    std::bitset<static_cast<uint8_t>(Failures::WITHOLD_RECONSTRUCTION_SHARES) + 1> failures_flags_;

    bool Failure(Failures f) const
    {
        return failures_flags_[static_cast<uint8_t>(f)];
    }
};

struct CabinetMember
{
    uint16_t                              muddle_port;
    NetworkManager                        network_manager;
    ProverPtr                             muddle_certificate;
    Muddle                                muddle;
    FaultyRbc                                   rbc;

    CabinetMember(uint16_t port_number, uint16_t index, RBC::CabinetMembers const &current_cabinet, const std::vector<FaultyRbc::Failures> &failures = {})
            : muddle_port{port_number}
            , network_manager{"NetworkManager" + std::to_string(index), 1}
            , muddle_certificate{CreateNewCertificate()}
            , muddle{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate, network_manager, true,
                     true}
            , rbc{muddle.AsEndpoint(), muddle_certificate->identity().identifier(), current_cabinet,
                  [this](ConstByteArray const &address, ConstByteArray const &payload) -> void {
                      OnRbcMessage(address, payload);
                  }, failures}
    {
        network_manager.Start();
        muddle.Start({muddle_port});
    }

    void OnRbcMessage(ConstByteArray const &address, ConstByteArray const &payload) {

    }

    ~CabinetMember()
    {
        muddle.Stop();
        muddle.Shutdown();
        network_manager.Stop();
    }
};

void GenerateTest(uint32_t cabinet_size, uint32_t expected_completion_size,
                  const std::vector<std::vector<FaultyRbc::Failures>> &failures       = {})
{
    RBC::CabinetMembers cabinet;

    std::vector<std::unique_ptr<CabinetMember>> committee;
    for (uint16_t ii = 0; ii < cabinet_size; ++ii)
    {
        auto port_number = static_cast<uint16_t>(9000 + ii);
        if (!failures.empty() && ii < failures.size())
        {
            committee.emplace_back(new CabinetMember{port_number, ii, cabinet, failures[ii]});
        }
        else
        {
            committee.emplace_back(new CabinetMember{port_number, ii, cabinet});
        }
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
            member->rbc.ResetCabinet();
        }

        // Start at DKG
        {
            for (auto &member : committee)
            {
                member->dkg.BroadcastShares();
            }

            // Loop until everyone is finished with DKG
            uint32_t pp = 0;
            while (pp < cabinet_size)
            {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                for (auto qq = pp; qq < cabinet_size; ++qq)
                {
                    if (!committee[qq]->dkg.finished())
                    {
                        break;
                    }
                    else
                    {
                        ++pp;
                    }
                }
            }
        }
}

