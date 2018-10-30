//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "core/byte_array/encoders.hpp"
#include "helper_functions.hpp"
#include "network/tcp/loopback_server.hpp"
#include "network/tcp/tcp_client.hpp"
#include "network/tcp/tcp_server.hpp"
#include "core/commandline/params.hpp"

// Test of the client. We use an echo server from the asio examples to ensure
// that any problems must be in the NM or the client. In this way we can test
// transmit and receive functionality by looping back

using namespace fetch::network;
using namespace fetch::common;
using namespace fetch::byte_array;

static constexpr char const *LOGGING_NAME = "TcpClientStressTests";
static constexpr std::size_t MANY_CYCLES  = 200;
static constexpr std::size_t MID_CYCLES   = 50;

std::atomic<std::size_t> clientReceivedCount{0};
bool                     printingClientResponses = false;

// Simple function to check if port is free
bool TcpServerAt(uint16_t port)
{
  try
  {
    fetch::network::LoopbackServer echoServer(port);
  }
  catch (...)
  {
    return true;
  }
  return false;
}

uint16_t GetOpenPort()
{
  uint16_t port = 8090;
  while (true)
  {
    if (TcpServerAt(port))
    {
      port++;
      std::cerr << "Trying next port for absence" << std::endl;
    }
    else
    {
      break;
    }
  }
  return port;
}

// Basic client increments on received messages
class Client : public TCPClient
{
public:
  Client(std::string const &host, std::string const &port, NetworkManager &nmanager)
    : TCPClient(nmanager)
  {
    Connect(host, port);
    this->OnMessage([](message_type const &value) {
      if (printingClientResponses)
      {
        std::cerr << "Client received: " << clientReceivedCount << std::endl;
        for (std::size_t i = 0; i < std::min(std::size_t(30), value.size()); ++i)
        {
          std::cerr << value[i];
        }
        std::cerr << std::endl;
      }

      clientReceivedCount++;
    });
  }

  ~Client()
  {
    TCPClient::Cleanup();
  }
};

// Client takes a while to process
class SlowClient : public TCPClient
{
public:
  SlowClient(std::string const &host, std::string const &port, NetworkManager &nmanager)
    : TCPClient(nmanager)
  {
    Connect(host, port);
    this->OnMessage([](message_type const &value) {
      if (printingClientResponses)
      {
        std::cerr << "Client received: " << clientReceivedCount << std::endl;
        for (std::size_t i = 0; i < std::min(std::size_t(30), value.size()); ++i)
        {
          std::cerr << value[i];
        }
        std::cerr << std::endl;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      clientReceivedCount++;
    });
  }

  ~SlowClient()
  {
    TCPClient::Cleanup();
  }
};

std::vector<message_type> globalMessages{};
std::mutex                mutex_;

// Client saves messages
class VerifyClient : public TCPClient
{
public:
  VerifyClient(std::string const &host, std::string const &port, NetworkManager &nmanager)
    : TCPClient(nmanager)
  {
    Connect(host, port);
    this->OnMessage([](message_type const &value) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        globalMessages.push_back(std::move(value));
      }
      clientReceivedCount++;
    });
  }

  ~VerifyClient()
  {
    TCPClient::Cleanup();
  }
};

// Create random data for testing
std::vector<message_type> CreateTestData(size_t index)
{
  std::size_t messagesToSend = MID_CYCLES;
  globalMessages.clear();
  globalMessages.reserve(messagesToSend);
  bool smallPackets = true;

  std::vector<message_type> sendData;

  if (index >= 5)
  {
    smallPackets = false;
  }

  for (std::size_t i = 0; i < messagesToSend; ++i)
  {
    std::size_t packetSize = 1000000;
    if (smallPackets)
    {
      packetSize = 100;
    }

    message_type arr;
    arr.Resize(packetSize);
    for (std::size_t z = 0; z < arr.size(); z++)
    {
      arr[z] = uint8_t(GetRandom());
    }
    sendData.push_back(arr);
  }

  return sendData;
}

template <std::size_t N = 1>
void TestCase1_InvalidTargetDeadNetman(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 1. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open a connection to a port\
    that doesn't exist (NM dead)"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager(N);
    Client         client(host, std::to_string(emptyPort), nmanager);
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase2_InvalidTarget(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 2. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open a connection to a port\
    that doesn't exist (NM alive)"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager(N);
    nmanager.Start();
    Client client(host, std::to_string(emptyPort), nmanager);
    nmanager.Stop();
  }
  std::cerr << "Success." << std::endl;
}

// Testcase 3 was a dupe of TC4

template <std::size_t N = 1>
void TestCase4_InvalidTargetFlakeyNetman(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 4. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open a connection to a port that\
    doesn't exist (NM jittering)"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  std::cerr << "starting" << std::endl;
  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager(N);
    if (index % 2 == 0)
    {
      nmanager.Start();
    }
    Client client(host, std::to_string(emptyPort), nmanager);
    if (index % 3 == 0)
    {
      nmanager.Stop();
    }
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase5_ValidTargetDeadNetman(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 5. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open a connection to a port that\
    does exist (NM dead)"
            << std::endl;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager(N);
    Client         client(host, port, nmanager);
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase6_ValidTargetLiveNetman(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 6. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open a connection to a\
    port that does exist (NM alive)"
            << std::endl;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager(N);
    nmanager.Start();
    Client client(host, port, nmanager);
    nmanager.Stop();
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase7_ValidTargetFlakeyNetman(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 7. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open a connection to a\
    port that does exist (NM jittering)"
            << std::endl;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager(N);
    if (index % 2 == 0)
    {
      nmanager.Start();
    }
    Client client(host, port, nmanager);
    if (index % 3 == 0)
    {
      nmanager.Stop();
    }
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase8_MulticonnsValidPort(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 8. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open multiple\
    connections to a port that does exist (move constr)"
            << std::endl;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));
  std::vector<Client>            clients;

  NetworkManager nmanager(N);
  nmanager.Start();
  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    clients.push_back(Client(host, port, nmanager));
  }
  nmanager.Stop();
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase9_AsyncMulticonnsValidPort(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 9. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open multiple\
    connections to a port that does exist, async"
            << std::endl;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));

  for (std::size_t index = 0; index < 10; ++index)
  {
    std::cerr << "Iteration: " << index << std::endl;
    NetworkManager nmanager(N);
    nmanager.Start();
    std::atomic<std::size_t> threadCount{0};
    std::size_t              iterations = MID_CYCLES;

    for (std::size_t i = 0; i < iterations; ++i)
    {
      std::thread([&host, &port, &nmanager, &threadCount] {
        NetworkManager managerCopy = nmanager;
        auto           i           = std::make_shared<Client>(host, port, managerCopy);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        i->Send("test");
        threadCount++;
      })
          .detach();
    }
    if (index % 2 == 0)
    {
      nmanager.Stop();
    }

    while (threadCount != iterations)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase10_NetmanDiesBeforeClients(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 10. Threads: " << N << std::endl;
  std::cerr << "Info: (Legacy) Usually breaks due to the NM being destroyed "
               "before the clients"
            << std::endl;

  for (std::size_t index = 0; index < MID_CYCLES; ++index)
  {
    std::vector<Client> clients;
    NetworkManager      nmanager(N);
    nmanager.Start();

    for (std::size_t j = 0; j < 4; ++j)
    {
      clients.push_back(Client(host, port, nmanager));
    }

    nmanager.Stop();

    for (std::size_t j = 0; j < 4; ++j)
    {
      clients.push_back(Client(host, port, nmanager));
    }
    nmanager.Start();
    if (index % 2)
    {
      nmanager.Stop();
    }
    if (index % 3)
    {
      nmanager.Stop();
    }
    if (index % 5)
    {
      nmanager.Stop();
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
  std::cerr << "success" << std::endl;
}

template <std::size_t N = 1>
void TestCase11_CheckAllMessagesResponded(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 11. Threads: " << N << std::endl;
  std::cerr << "Info: Bouncing messages off echo/loopback server and counting them" << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cerr << "Iteration: " << i << std::endl;
    fetch::network::LoopbackServer echoServer(emptyPort);
    NetworkManager                 nmanager(N);
    nmanager.Start();
    Client client(host, std::to_string(emptyPort), nmanager);

    while (!client.is_alive())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::size_t currentCount   = clientReceivedCount;
    auto        t1             = TimePoint();
    std::size_t messagesToSend = MANY_CYCLES;

    for (std::size_t j = 0; j < messagesToSend; ++j)
    {
      std::string mess{"Hello: "};
      mess += std::to_string(i);
      std::thread([&client, mess]() { client.Send(mess); }).detach();
    }

    while (clientReceivedCount != currentCount + messagesToSend)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));

      if (printingClientResponses)
      {
        std::cerr << "Waiting for messages to be rec. " << clientReceivedCount << " of "
                  << currentCount + messagesToSend << std::endl;
      }
    }

    auto t2 = TimePoint();

    if (printingClientResponses)
    {
      std::cerr << "Time for " << messagesToSend << " calls: " << TimeDifference(t1, t2)
                << std::endl;
    }

    nmanager.Stop();
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase12_CheckAllMessagesRespondedSlow(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 12. Threads: " << N << std::endl;
  std::cerr << "Info: Bouncing messages off echo/loopback\
    server and counting them, slow client "
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t i = 0; i < 5; ++i)
  {
    std::cerr << "Iteration: " << i << std::endl;
    fetch::network::LoopbackServer echoServer(emptyPort);
    NetworkManager                 nmanager(N);
    nmanager.Start();
    SlowClient client(host, std::to_string(emptyPort), nmanager);

    while (!client.is_alive())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::size_t currentCount   = clientReceivedCount;
    auto        t1             = TimePoint();
    std::size_t messagesToSend = MID_CYCLES;

    for (std::size_t j = 0; j < messagesToSend; ++j)
    {
      std::string mess{"Hello: "};
      mess += std::to_string(i);
      std::thread([&client, mess]() { client.Send(mess); }).detach();
    }

    while (clientReceivedCount != currentCount + messagesToSend)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));

      if (printingClientResponses)
      {
        std::cerr << "Waiting for messages to be rec. " << clientReceivedCount << " of "
                  << currentCount + messagesToSend << std::endl;
      }
    }

    auto t2 = TimePoint();

    if (printingClientResponses)
    {
      std::cerr << "Time for " << messagesToSend << " calls: " << TimeDifference(t1, t2)
                << std::endl;
    }

    nmanager.Stop();
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase13_CheckMessageResponseOrdering(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 13. Threads: " << N << std::endl;
  std::cerr << "Info: Bouncing messages off echo/loopback\
    server and checking ordering"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cerr << "Iteration: " << i << std::endl;
    NetworkManager nmanager(N);
    nmanager.Start();
    fetch::network::LoopbackServer echoServer(emptyPort);
    VerifyClient                   client(host, std::to_string(emptyPort), nmanager);

    while (!client.is_alive())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Precreate data, this handles clearing global counter etc.
    std::vector<message_type> sendData = CreateTestData(i);

    std::size_t expectCount = clientReceivedCount;
    auto        t1          = TimePoint();

    for (auto &dat : sendData)
    {
      client.Send(dat);
      expectCount++;
    }

    while (clientReceivedCount != expectCount)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));

      if (printingClientResponses)
      {
        std::cerr << "Waiting for messages to be rec. " << clientReceivedCount << " of "
                  << expectCount << std::endl;
      }
    }

    auto t2 = TimePoint();

    if (printingClientResponses)
    {
      std::cerr << "Time for " << sendData.size() << " calls: " << TimeDifference(t1, t2)
                << std::endl;
    }

    // Verify we transmitted correctly
    if (globalMessages.size() == 0)
    {
      std::cerr << "Failed to receive messages" << std::endl;
      exit(1);
    }

    if (globalMessages.size() != sendData.size())
    {
      std::cerr << "Failed to receive all messages" << std::endl;
      exit(1);
    }

    for (std::size_t i = 0; i < globalMessages.size(); ++i)
    {
      if (globalMessages[i] != sendData[i])
      {
        std::cerr << "Failed to verify messages" << std::endl;
        exit(1);
      }
    }
    nmanager.Stop();
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase14_CheckMessageResponseOrderingMulticon(std::string host, std::string port)
{
  std::cerr << "\nTEST CASE 14. Threads: " << N << std::endl;
  std::cerr << "Info: Bouncing messages off echo/loopback\
    server and checking ordering, multiple clients"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t index = 0; index < 10; ++index)
  {
    std::cerr << "Iteration: " << index << std::endl;
    fetch::network::LoopbackServer echoServer(emptyPort);
    NetworkManager                 nmanager(N);
    nmanager.Start();
    std::vector<std::shared_ptr<VerifyClient>> clients;

    for (std::size_t i = 0; i < 5; ++i)
    {
      clients.push_back(std::make_shared<VerifyClient>(host, std::to_string(emptyPort), nmanager));
    }

    // Precreate data, this handles clearing global counter etc.
    std::vector<message_type> sendData = CreateTestData(index);

    for (auto const &i : clients)
    {
      while (!i->is_alive())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }

    std::size_t expectCount = clientReceivedCount;
    auto        t1          = TimePoint();

    for (std::size_t k = 0; k < sendData.size(); ++k)
    {
      auto &client = clients[k % clients.size()];
      auto &data   = sendData[k];
      std::thread([client, &data]() { client->Send(data); }).detach();
      expectCount++;
    }

    while (clientReceivedCount != expectCount)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));

      if (printingClientResponses)
      {
        std::cerr << "Waiting for messages to be rec. " << clientReceivedCount << " of "
                  << expectCount << std::endl;
      }
    }

    auto t2 = TimePoint();

    if (printingClientResponses)
    {
      std::cerr << "Time for " << sendData.size() << " calls: " << TimeDifference(t1, t2)
                << std::endl;
    }

    // Verify we transmitted correctly
    if (globalMessages.size() == 0)
    {
      std::cerr << "Failed to receive messages" << std::endl;
      exit(1);
    }

    if (globalMessages.size() != sendData.size())
    {
      std::cerr << "Failed to receive all messages" << std::endl;
      exit(1);
    }

    // We need to sort since this will not be ordered
    std::sort(globalMessages.begin(), globalMessages.end());
    std::sort(sendData.begin(), sendData.end());

    for (std::size_t i = 0; i < globalMessages.size(); ++i)
    {
      if (globalMessages[i] != sendData[i])
      {
        std::cerr << "Failed to verify messages" << std::endl;
        exit(1);
      }
    }

    nmanager.Stop();
  }
  std::cerr << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase15_KilledDuringTransmitMulticon(std::string host, std::string const &port)
{
  std::cerr << "\nTEST CASE 15. Threads: " << N << std::endl;
  std::cerr << "Info: Killing during transmission, multiple clients" << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cerr << "Iteration: " << i << std::endl;
    fetch::network::LoopbackServer echoServer(emptyPort);
    NetworkManager                 nmanager(N);
    nmanager.Start();
    std::vector<std::shared_ptr<VerifyClient>> clients;

    for (std::size_t i = 0; i < 5; ++i)
    {
      clients.push_back(std::make_shared<VerifyClient>(host, std::to_string(emptyPort), nmanager));
    }

    std::size_t messagesToSend = MID_CYCLES;
    globalMessages.clear();
    globalMessages.reserve(messagesToSend);

    for (auto i : clients)
    {
      while (!i->is_alive())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }

    // Precreate data
    std::vector<message_type> sendData;

    for (uint8_t k = 0; k < 8; k++)
    {
      std::size_t  packetSize = 1000;
      message_type arr;
      arr.Resize(packetSize);
      for (std::size_t z = 0; z < arr.size(); z++)
      {
        arr[z] = k;
      }

      sendData.push_back(arr);
    }

    for (std::size_t j = 0; j < messagesToSend; ++j)
    {
      for (auto const &i : sendData)
      {
        for (auto const &client : clients)
        {
          std::thread([client, i]() { client->Send(i); }).detach();
        }
      }
    }
    if (i % 2)
    {
      nmanager.Stop();
    }
  }
  std::cerr << "Success." << std::endl;
}

int main(int argc, char *argv[])
{
  std::string host       = "localhost";
  uint16_t    portNumber = 8080;
  bool all = false;
  std::size_t iterations = 1;

  fetch::commandline::Params params;

  params.add(iterations, "iterations", "Set the number of iterations.", std::size_t(1));
  params.add(host, "host", "Set the hostname to use.", std::string("localhost"));
  params.add(portNumber, "port", "Set the port to run using.", uint16_t(8080));
  params.add(all, "all", "Run ALL the tests, not just the sanity checkers.", false);

  params.Parse(argc, argv);

  std::string port       = std::to_string(portNumber);

  FETCH_LOG_INFO(LOGGING_NAME, "Running test iterations: ", iterations);

  for (std::size_t i = 0; i < iterations; ++i)
  {
    // Do most likely to fail test first
    if (all)
    {
      TestCase9_AsyncMulticonnsValidPort<1>(host, port);
    }
    TestCase9_AsyncMulticonnsValidPort<10>(host, port);

    if (all)
    {
      //TestCase8_MulticonnsValidPort<1>(host, port);
      TestCase11_CheckAllMessagesResponded<1>(host, port);
      TestCase13_CheckMessageResponseOrdering<1>(host, port);
    }

    if (all)
    {
      TestCase1_InvalidTargetDeadNetman<1>(host, port);
      TestCase2_InvalidTarget<1>(host, port);
      TestCase4_InvalidTargetFlakeyNetman<1>(host, port);
      TestCase5_ValidTargetDeadNetman<1>(host, port);
      TestCase6_ValidTargetLiveNetman<1>(host, port);
      TestCase7_ValidTargetFlakeyNetman<1>(host, port);
      TestCase12_CheckAllMessagesRespondedSlow<1>(host, port);
      TestCase13_CheckMessageResponseOrdering<1>(host, port);
    }

    if (all)
    {
      //TestCase8_MulticonnsValidPort<10>(host, port);
      TestCase11_CheckAllMessagesResponded<10>(host, port);
      TestCase13_CheckMessageResponseOrdering<10>(host, port);
    }

    // Save runtime by only doing the multiple-thread testcases.
    TestCase1_InvalidTargetDeadNetman<10>(host, port);
    TestCase2_InvalidTarget<10>(host, port);
    TestCase4_InvalidTargetFlakeyNetman<10>(host, port);
    TestCase5_ValidTargetDeadNetman<10>(host, port);
    TestCase6_ValidTargetLiveNetman<10>(host, port);
    TestCase7_ValidTargetFlakeyNetman<10>(host, port);
    TestCase12_CheckAllMessagesRespondedSlow<10>(host, port);

    TestCase14_CheckMessageResponseOrderingMulticon<10>(host, port);
    TestCase15_KilledDuringTransmitMulticon<10>(host, port);
  }

  std::cerr << "finished all tests" << std::endl;
  return 0;
}
