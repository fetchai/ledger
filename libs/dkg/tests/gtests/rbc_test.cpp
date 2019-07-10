#include "dkg/rbc.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "gtest/gtest.h"

#include <iostream>
#include "network/muddle/muddle.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"

using namespace fetch::network;
using namespace fetch::crypto;
using namespace fetch::muddle;
using namespace fetch::dkg::rbc;

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate       = fetch::crypto::Prover;
using CertificatePtr    = std::shared_ptr<Certificate>;
using Address         = fetch::muddle::Packet::Address;

ProverPtr CreateNewCertificate()
{
    using Signer    = fetch::crypto::ECDSASigner;
    using SignerPtr = std::shared_ptr<Signer>;

    SignerPtr certificate        = std::make_shared<Signer>();

    certificate->GenerateKeys();

    return certificate;
}

int main()
{
    std::size_t number_of_threads = 1;
    uint16_t muddle_port_0 = 8000;
    uint16_t muddle_port_1 = 8001;
    uint16_t muddle_port_2 = 8002;
    uint16_t muddle_port_3 = 8003;

    // Muddle '0'
    NetworkManager network_manager0{"NetworkManager0", number_of_threads};
    auto muddle_certificate_0 = CreateNewCertificate();
    Muddle muddle0{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate_0, network_manager0, true, true};
    network_manager0.Start();

    // Muddle '1'
    NetworkManager network_manager1{"NetworkManager1", number_of_threads};
    auto muddle_certificate_1 = CreateNewCertificate();
    Muddle muddle1{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate_1, network_manager1, true, true};
    network_manager1.Start();

    // Muddle '2'
    NetworkManager network_manager2{"NetworkManager2", number_of_threads};
    auto muddle_certificate_2 = CreateNewCertificate();
    Muddle muddle2{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate_2, network_manager2, true, true};
    network_manager2.Start();

    // Muddle '3'
    NetworkManager network_manager3{"NetworkManager3", number_of_threads};
    auto muddle_certificate_3 = CreateNewCertificate();
    Muddle muddle3{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate_3, network_manager3, true, true};
    network_manager3.Start();

    muddle0.Start({muddle_port_0});
    muddle1.Start({muddle_port_1});
    muddle2.Start({muddle_port_2});
    muddle3.Start({muddle_port_3});

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Connect muddles together (localhost for this example)
    muddle0.AddPeer(fetch::network::Uri{"tcp://127.0.0.1:8001"});
    muddle0.AddPeer(fetch::network::Uri{"tcp://127.0.0.1:8002"});
    muddle0.AddPeer(fetch::network::Uri{"tcp://127.0.0.1:8003"});

    muddle1.AddPeer(fetch::network::Uri{"tcp://127.0.0.1:8002"});
    muddle1.AddPeer(fetch::network::Uri{"tcp://127.0.0.1:8003"});

    muddle2.AddPeer(fetch::network::Uri{"tcp://127.0.0.1:8003"});

    while(muddle1.AsEndpoint().GetDirectlyConnectedPeers().size() != 3 or muddle2.AsEndpoint().GetDirectlyConnectedPeers().size() != 3
            or muddle3.AsEndpoint().GetDirectlyConnectedPeers().size() != 3);
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }


    RBC::CabinetMembers cabinet{muddle_certificate_0->identity().identifier(), muddle_certificate_1->identity().identifier(), muddle_certificate_2->identity().identifier(),
                                muddle_certificate_3->identity().identifier()};

    //Start at RBC for each muddle
    {
        uint32_t threshold{1};
        RBC rbc0{muddle0.AsEndpoint(), muddle_certificate_0->identity().identifier(), cabinet, threshold};
        RBC rbc1{muddle1.AsEndpoint(), muddle_certificate_1->identity().identifier(), cabinet, threshold};
        RBC rbc2{muddle2.AsEndpoint(), muddle_certificate_2->identity().identifier(), cabinet, threshold};
        RBC rbc3{muddle3.AsEndpoint(), muddle_certificate_3->identity().identifier(), cabinet, threshold};

        //rbc0 sends a broacast
        rbc0.sendRBroadcast("hello");

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    //Sometimes a seg fault appears at this stage.
    muddle0.Stop();
    muddle1.Stop();
    muddle2.Stop();
    muddle3.Stop();
}