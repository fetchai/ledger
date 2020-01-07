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

#include "kademlia/peer_info.hpp"
#include "kademlia/table.hpp"
#include "muddle.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "router.hpp"

#include "gtest/gtest.h"
using namespace fetch::muddle;
using namespace fetch;

Address FakeAddress(uint64_t i)
{
  Address        ret;
  crypto::SHA256 hasher;

  hasher.Update(reinterpret_cast<uint8_t *>(&i), sizeof(uint64_t));
  ret = hasher.Final();

  return ret;
}

PeerInfo GeneratePeerInfo(uint64_t i)
{
  PeerInfo ret;

  ret.address = FakeAddress(i);
  ret.uri.Parse("tcp://127.0.0.1:" + std::to_string(i));

  return ret;
}

TEST(Kademlia, TableRecovery)
{
  uint64_t                             N = 1000;
  std::vector<fetch::muddle::PeerInfo> generated_info;

  // Generating table
  {
    KademliaTable table{FakeAddress(N + 1), fetch::muddle::NetworkId("TEST")};
    table.SetCacheFile("test.peer_table");
    for (uint64_t i = 0; i < N; ++i)
    {
      auto info = GeneratePeerInfo(i);
      generated_info.push_back(info);
      table.ReportExistence(info, FakeAddress(N + 1));
    }

    table.Dump();
  }

  // Loading table
  {
    KademliaTable table{FakeAddress(N + 1), fetch::muddle::NetworkId("TEST")};

    table.SetCacheFile("test.peer_table");
    table.Load();
    EXPECT_EQ(table.size(), N);

    // TODO(tfr): Test contents
  }
}
