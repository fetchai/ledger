#include"network/thread_manager.hpp"
#include"./network_benchmark_service.hpp"
#include"./node_basic.hpp"
#include"chain/transaction.hpp"
#include"../tests/include/helper_functions.hpp"

using namespace fetch;
using namespace fetch::network_benchmark;
using namespace fetch::serializers;

int main(int argc, char **argv)
{

  fetch::network::ThreadManager *tm  = new fetch::network::ThreadManager(30);

  {
    int seed = 0;
    if (argc > 1)
    {
      std::stringstream s(argv[1]);
      s >> seed;
    }

    uint16_t tcpPort  = uint16_t(9080+seed);
    uint16_t httpPort = uint16_t(8080+seed);

    fetch::network_benchmark::NetworkBenchmarkService<NodeBasic> serv(tm, tcpPort, httpPort);
    tm->Start();

    std::cout << "press any key to quit" << std::endl;
    std::cin >> seed;
  }

  tm->Stop();
  delete tm;
  return 0;
}
