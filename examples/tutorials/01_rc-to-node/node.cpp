#include<iostream>
#include<functional>
#include"node.hpp"

#include"commandline/parameter_parser.hpp"
#include <iostream>
#include <memory>
using namespace fetch::commandline;

int main(int argc, char const **argv) {

  ParamsParser params;
  params.Parse(argc, argv);

  if(params.arg_size() <= 2) {
    std::cout << "usage: ./" << argv[0] << " [port] [info]" << std::endl;
    return -1;
  }

  std::cout << "Starting service on " << params.GetArg<uint16_t>(1) << std::endl;
  FetchService serv( params.GetArg<uint16_t>(1), params.GetArg(2));
  serv.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;                                       
  std::cin >> dummy;
  
  serv.Stop();

  return 0;
}
