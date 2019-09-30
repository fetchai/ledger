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

#include "agentapi/agentapi.hpp"
#include "crypto/ecdsa.hpp"
#include <iostream>
#include <set>

using namespace fetch::service;
using namespace fetch::byte_array;
using namespace fetch::agent;
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

// First we make a service implementation
int main()
{
  auto                    muddle_certificate = CreateNewCertificate();
  network::NetworkManager network_manager{"SearchNetworkManager", 1};
  muddle::MuddlePtr       muddle =
      muddle::CreateMuddle("AGEN", muddle_certificate, network_manager, "127.0.0.1");

  network_manager.Start();
  muddle->Start({static_cast<uint16_t>(1337)});

  Mailbox  mailbox{muddle};
  AgentAPI serv(muddle, mailbox);

  std::string search_for;
  std::cout << "Enter a string to search the AEAs for this string" << std::endl;
  while (!std::cin.eof())
  {
    std::cout << "> " << std::flush;
    std::getline(std::cin, search_for);
  }

  std::cout << std::endl;
  std::cout << "Bye ..." << std::endl;
  network_manager.Stop();

  return 0;
}
