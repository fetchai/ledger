#include "../tests/include/helper_functions.hpp"
#include "./mine_node_basic.hpp"
#include "./network_mine_test_service.hpp"
#include "network/management/network_manager.hpp"

using namespace fetch;
using namespace fetch::network_mine_test;
using namespace fetch::serializers;

int main(int argc, char **argv)
{

  fetch::network::NetworkManager tm(30);

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
