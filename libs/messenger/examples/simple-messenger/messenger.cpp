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

#include "core/commandline/parameter_parser.hpp"
#include "core/serializers/base_types.hpp"
#include "crypto/ecdsa.hpp"
#include "messenger/messenger_prototype.hpp"

#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"

#include <iostream>

using namespace fetch::commandline;
using namespace fetch::service;
using namespace fetch::byte_array;
using namespace fetch::messenger;
using namespace fetch::network;
using namespace fetch;

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

class SimpleMessenger : public MessengerPrototype
{
public:
  using MessengerPrototype::MessengerPrototype;
};

int main(int argc, char **argv)
{

  ParamsParser params;
  params.Parse(argc, argv);

  auto                    muddle_certificate = CreateNewCertificate();
  network::NetworkManager network_manager{"MessengerNetworkManager", 1};
  muddle::MuddlePtr       muddle =
      muddle::CreateMuddle("AGEN", muddle_certificate, network_manager, "127.0.0.1");

  network_manager.Start();
  muddle->Start({"tcp://127.0.0.1:1337"}, {static_cast<uint16_t>(1338)});

  // Waiting until connection
  while (muddle->GetDirectlyConnectedPeers().empty())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  auto            messenger_api_addresses = muddle->GetDirectlyConnectedPeers();
  SimpleMessenger messenger(muddle, messenger_api_addresses);

  std::string search_for;
  std::cout << "Messenger ready:" << std::endl;
  while (!std::cin.eof())
  {
    std::cout << "> " << std::flush;
    std::getline(std::cin, search_for);
    if (search_for == "register")
    {
      messenger.Register();
    }
    else
    {
      messenger.FindAgents("semantic", search_for);
    }
  }

  std::cout << std::endl;
  std::cout << "Bye ..." << std::endl;
  network_manager.Stop();

  return 0;
}
