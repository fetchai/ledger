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

#include "helper_functions.hpp"
#include "mine_node_basic.hpp"
#include "network/management/network_manager.hpp"
#include "network_mine_test_service.hpp"

using namespace fetch;
using namespace fetch::network_mine_test;
using namespace fetch::serializers;

int main(int argc, char **argv)
{
  fetch::network::NetworkManager tm{"NetMgr", 30};

  {
    int seed = 0;
    if (argc > 1)
    {
      std::stringstream s(argv[1]);
      s >> seed;
    }

    uint16_t tcpPort  = uint16_t(9080 + seed);
    uint16_t httpPort = uint16_t(8080 + seed);

    fetch::network_mine_test::NetworkMineTestService<MineNodeBasic> serv(tm, tcpPort, httpPort);
    tm.Start();

    std::cout << "press any key to quit" << std::endl;
    std::cin >> seed;
  }

  tm.Stop();
  return 0;
}
