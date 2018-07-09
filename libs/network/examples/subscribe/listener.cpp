#include"network/service/function.hpp"
#include"network/service/client.hpp"
#include"./protocols/fetch_protocols.hpp"
#include"./protocols/subscribe/commands.hpp"
#include<iostream>

using namespace fetch::service;
using namespace fetch::protocols;

int main(int argc, char const **argv) {

  uint16_t port = 8080;

  fetch::network::ThreadManager tm;
  // Create a client connection to the server
  fetch::network::TCPClient connection(tm);
  connection.Connect("localhost", 8080);
  
  ServiceClient client(connection, tm);  
  

  tm.Start();

  while(!client.is_alive())
  {
    std::cout << "Waiting for client to connect" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::cout << "Listening to " << port << std::endl;
  int count{0};

  // We define a function for what we want to happen when the remote has a new message.
  // Count incremented by reference
  auto handle1 = client.Subscribe(FetchProtocols::SUBSCRIBE_PROTO, SubscribeProto::NEW_MESSAGE,
      new Function< void(std::string) >([&count](std::string const&msg)
        {
          std::cout << "Got message: " << msg << std::endl;
          count++;
        }));

  while(count < 5)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  std::cout << "Leaving" << std::endl;
  client.Unsubscribe( handle1 ); // Example of how we can unsubscribe from listening

  return 0;
}
