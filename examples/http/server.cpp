#include<iostream>
#include"http/server.hpp"
using namespace fetch::http;

int main() 
{
  fetch::network::ThreadManager tm(8);  
  HTTPServer server(8080, &tm);

  tm.Start();
  
  std::cout << "Ctrl-C to stop" << std::endl;
  while(true) 
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
  }

  tm.Stop();
  
  return 0;
}
