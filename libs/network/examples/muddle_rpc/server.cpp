#include "muddle_rpc.hpp"

#include <iostream>
#include <thread>
#include <chrono>

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

using fetch::network::NetworkManager;
using fetch::network::Peer;
using fetch::muddle::Muddle;
using fetch::muddle::rpc::Server;

int main()
{
  NetworkManager nm(1);
  nm.Start();

  Muddle muddle{CreateKey(SERVER_PRIVATE_KEY), nm};
  muddle.Start({8000});

  Sample sample;
  SampleProtocol sample_protocol(sample);

  Server server{muddle.AsEndpoint(), 1};
  server.Add(1, &sample_protocol);

  for (;;)
  {
    sleep_for(milliseconds{500});
  }

  return 0;
}
