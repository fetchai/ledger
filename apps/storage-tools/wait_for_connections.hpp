#pragma once
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

#include "muddle/muddle_interface.hpp"

#include <chrono>

inline bool WaitForPeerConnections(fetch::muddle::MuddleInterface &net, std::size_t count)
{
  using namespace std::chrono_literals;

  bool success{false};

  FETCH_LOG_INFO("WaitForPeers", "Establishing connection(s) to peer...");

  for (;;)
  {
    auto const peers = net.GetDirectlyConnectedPeers();

    if (peers.empty())
    {
      std::this_thread::sleep_for(100ms);
    }
    else if (peers.size() == count)
    {
      success = true;
      break;
    }
    else
    {
      FETCH_LOG_WARN("WaitForPeers", "Multiple connections when only expecting one");
      break;
    }
  }

  FETCH_LOG_INFO("WaitForPeers", "Establishing connection(s) to peer...complete");

  return success;
}
