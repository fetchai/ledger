#include"oef/fetch_node_service.hpp"

using namespace fetch::fetch_node_service;

int main() {
  fetch::network::ThreadManager tm(8);
  FetchNodeService serv(8090, &tm); // Note, the rpc interface is on port 8090, the http interface is 8080

  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
