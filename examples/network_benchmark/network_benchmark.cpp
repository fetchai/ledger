#include"network/thread_manager.hpp"
#include"./network_benchmark_service.hpp"
#include"./node.hpp"
#include"./node_basic.hpp"
#include"chain/transaction.hpp"

using namespace fetch;
using namespace fetch::network_benchmark;
using namespace fetch::serializers;

int main(int argc, char **argv)
{

  auto trans = Node::NextTransaction();

  TypedByte_ArrayBuffer serializer;

  serializer << trans;

  fetch::logger.Info("Transaction size is: ", serializer.size());

  // Default to 10 threads, this can/will be changed by HTTP interface
  fetch::network::ThreadManager *tm = new fetch::network::ThreadManager(10);

  {
    int seed = 0;
    if (argc > 1)
    {
      std::stringstream s(argv[1]);
      s >> seed;
    }

    uint16_t tcpPort  = 9080+seed;
    uint16_t httpPort = 8080+seed;

    fetch::network_benchmark::NetworkBenchmarkService<Node> serv(tm, tcpPort, httpPort);
    tm->Start();
    //serv.Start(); // the python/http will do this

    std::cout << "press any key to quit" << std::endl;
    std::cin >> seed;
  }

  // TODO: (`HUT`) : investigate: tcp or http server doesn't like destructing before tm stopped
  tm->Stop();
  //delete tm;
  return 0;
}
