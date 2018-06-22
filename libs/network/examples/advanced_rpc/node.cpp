#include<iostream>
#include<functional>
#include"service.hpp"

#include"core/commandline/parameter_parser.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
using namespace fetch::commandline;
/**
 * Please see aea.cpp for documentation.
 **/
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


  // Here we create a feed of contious publication done through the implementation.
  // In our case, we make a clock that says tick/tock every half second.
  bool ticktock = false;
  while(true) {
    ticktock = !ticktock;
    if(ticktock) {
      serv.Tick();
      std::cout << "Tick" << std::endl;
    } else {
      serv.Tock();
      std::cout << "Tock" << std::endl;
    }
    
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
  }
  
  serv.Stop();

  return 0;
}
