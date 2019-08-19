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
#include "dkg/pre_dkg_sync.hpp"
#include "network/management/network_manager.hpp"
#include "muddle/muddle_interface.hpp"

// TODO(WK) Extract to library: .cpp file include == ++ungood
#include "../../apps/constellation/key_generator.cpp"
#include "../../apps/constellation/key_generator.hpp"

#include <iostream>

using namespace fetch;
using namespace fetch::crypto;
using namespace fetch::dkg;

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
  std::vector<std::string>                                            args;
  std::unordered_map<byte_array::ConstByteArray, fetch::network::Uri> peer_list;
  dkg::DkgService::CabinetMembers members{p2p_key->identity().identifier()};
  for (int i = 1; i < 4; ++i)
  {
    args.emplace_back(std::string{argv[i]});
  }
  peer_list.insert(
      {p2p_key->identity().identifier(), fetch::network::Uri{"tcp://127.0.0.1:" + args[1]}});
  for (int i = 4; i < argc - 1; i += 2)
  {
    byte_array::ConstByteArray address{argv[i + 1]};
    peer_list.insert({FromBase64(address), fetch::network::Uri{argv[i]}});
    members.insert(FromBase64(address));
  }

  // Muddle setup/gubbins
  fetch::network::NetworkManager network_manager{"NetworkManager", 2};
  fetch::core::Reactor           reactor{"ReactorName"};
  auto muddle = muddle::CreateMuddle("Test", p2p_key, network_manager, "127.0.0.1");

  std::unique_ptr<dkg::DkgService> dkg =
      std::make_unique<dkg::DkgService>(muddle->GetEndpoint(), p2p_key->identity().identifier());

  // Start networking etc.
  network_manager.Start();
  muddle->Start({}, {uint16_t(std::stoi(args[1]))});

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Connect and wait until everyone else has connected
  PreDkgSync sync(muddle->GetEndpoint(), 4);
  sync.ResetCabinet(peer_list);
  sync.Connect();
  while (!sync.ready())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  auto index = std::distance(members.begin(), members.find(p2p_key->identity().identifier()));
  FETCH_LOG_INFO(LOGGING_NAME, "Connected to peers - node ", index);

  // Reset cabinet in DKG
  dkg->ResetCabinet(members, uint32_t(std::stoi(args[2])));
  FETCH_LOG_INFO(LOGGING_NAME, "Resetting cabinet");

  // Machinery to drive the FSM - attach and begin!
  reactor.Attach(dkg->GetWeakRunnable());
  reactor.Start();

  std::this_thread::sleep_for(std::chrono::seconds(300));  // 1 min

  FETCH_LOG_INFO(LOGGING_NAME, "Finished. Quitting");

  return 0;
}
