#include<iostream>
#include"node.hpp"

int main() {
  FetchService serv(8080);
  serv.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;                                       
  std::cin >> dummy;
  
  serv.Stop();

  return 0;
}
