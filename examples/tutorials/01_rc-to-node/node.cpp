#include<iostream>
#include"node.hpp"

int main() {
  FetchService serv(8080, "lat=0.47, long=9");
  serv.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;                                       
  std::cin >> dummy;
  
  serv.Stop();

  return 0;
}
