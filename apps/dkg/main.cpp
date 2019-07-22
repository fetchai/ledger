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

#include "core/macros.hpp"
#include "core/reactor.hpp"

#include "dkg/dkg_service.hpp"

#include "network/management/network_manager.hpp"
#include "network/muddle/muddle.hpp"

#include "../../apps/constellation/key_generator.cpp"
#include "../../apps/constellation/key_generator.hpp"

using namespace fetch;
using namespace fetch::crypto;

int main(int argc, char **argv)
{
  // Create (or load from file) this node's identity (pub/private key)
  // and print it out for external tool to use
  auto p2p_key = fetch::GenerateP2PKey();

  // External tool needs this
  std::cout << p2p_key->identity().identifier().ToBase64() << std::endl;

  // The arguments for this program shall be : ./exe beacon_address port threshold peer1
  // priv_key_b64 peer2 priv_key_b64...
  if (argc == 1)
  {
    FETCH_LOG_INFO(
        "MAIN",
        "Usage: ./exe beacon_address port threshold peer1 priv_key_b64 peer2 priv_key_b64...");
    return 0;
  }

  // Parse command line args
  std::vector<std::string> args;
  std::vector<std::string> peer_ip_addresses;
  for (int i = 1; i < argc; ++i)
  {
    args.emplace_back(std::string{argv[i]});

    if (i >= 4)
    {
      if (!(i & 0x1))
      {
        peer_ip_addresses.push_back(args.back());
      }
    }
  }

  // Muddle setup/gubbins
  fetch::network::NetworkManager network_manager{"NetworkManager", 2};
  fetch::core::Reactor           reactor{"ReactorName"};
  auto muddle = std::make_shared<fetch::muddle::Muddle>(fetch::muddle::NetworkId{"TestDKGNetwork"},
                                                        p2p_key, network_manager, true, true);

  std::shared_ptr<dkg::DkgService> dkg =
      std::make_unique<dkg::DkgService>(muddle->AsEndpoint(), p2p_key->identity().identifier());

  // Start networking etc.
  network_manager.Start();
  muddle->Start({uint16_t(std::stoi(args[1]))});

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (auto const &i : peer_ip_addresses)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Telling muddle to connect to peer: ", i);
    muddle->AddPeer(fetch::network::Uri{i});
  }

  // Important! Block until there are peers - dkg not resistant to this case
  while (muddle->AsEndpoint().GetDirectlyConnectedPeers().size() != peer_ip_addresses.size())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  FETCH_LOG_INFO(LOGGING_NAME,
                 "Connected peers: ", muddle->AsEndpoint().GetDirectlyConnectedPeers().size());
  FETCH_LOG_INFO(LOGGING_NAME, "expected connected peers: ", peer_ip_addresses.size());

  // Create cabinet
  dkg::DkgService::CabinetMembers members{p2p_key->identity().identifier()};
  for (auto const &address : muddle->AsEndpoint().GetDirectlyConnectedPeers())
  {
    members.insert(address);
  }
  assert(members.size() == peer_ip_addresses.size() + 1);
  auto index = std::distance(members.begin(), members.find(p2p_key->identity().identifier()));
  FETCH_LOG_INFO(LOGGING_NAME, "Node ", index, " resetting cabinet");

  std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  dkg->ResetCabinet(members, uint32_t(std::stoi(args[2])));

  // Machinery to drive the FSM - attach and begin!
  reactor.Attach(dkg->GetWeakRunnable());
  reactor.Start();

  std::this_thread::sleep_for(std::chrono::seconds(1000));  // 1 min

  FETCH_LOG_INFO(LOGGING_NAME, "Finished. Quitting");

  return 0;
}
