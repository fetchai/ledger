#include"oef/service_oef.hpp"

using namespace fetch::service_oef;

int main() {
  fetch::network::ThreadManager tm(8);
  ServiceOEF serv(8090, &tm); // Note, the rpc interface is on port 8090, the http interface is 8080

  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
