//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/encoders.hpp"
#include "core/commandline/params.hpp"
#include "network/tcp/loopback_server.hpp"
#include "network/tcp/tcp_client.hpp"
#include "network/tcp/tcp_server.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>

// Test of the client. We use an echo server from the asio examples to ensure
// that any problems must be in the NM or the client. In this way we can test
// transmit and receive functionality by looping back

namespace {

using namespace fetch::network;
using namespace fetch::byte_array;

constexpr std::size_t MANY_CYCLES = 200;
constexpr std::size_t MID_CYCLES  = 50;
constexpr std::size_t FEW_CYCLES  = 10;

std::string host       = "localhost";
uint16_t    portNumber = 8080;
std::string port       = std::to_string(portNumber);
std::size_t N          = 1;

std::atomic<std::size_t> clientReceivedCount{0};

// Simple function to check if port is free
bool TcpServerAt(uint16_t port_param)
{
  try
  {
    fetch::network::LoopbackServer echoServer(port_param);
  }
  catch (...)
  {
    return true;
  }
  return false;
}

uint16_t GetOpenPort()
{
  uint16_t port_number = 8090;
  while (true)
  {
    if (TcpServerAt(port_number))
    {
      port_number++;
      std::cerr << "Trying next port for absence" << std::endl;
    }
    else
    {
      break;
    }
  }
  return port_number;
}

// Basic client increments on received messages
class Client : public TCPClient
{
public:
  Client(std::string const &host_name, std::string const &port_number, NetworkManager &nmanager)
    : TCPClient(nmanager)
  {
    Connect(host_name, port_number);
    this->OnMessage([](MessageBuffer const &) { clientReceivedCount++; });
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
  SlowClient(std::string const &host_name, std::string const &port_number, NetworkManager &nmanager)
    : TCPClient(nmanager)
  {
    Connect(host_name, port_number);
    this->OnMessage([](MessageBuffer const &) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      clientReceivedCount++;
    });
  }

  ~SlowClient()
  {
    TCPClient::Cleanup();
  }
};

std::vector<MessageBuffer> globalMessages{};
std::mutex                 mutex_;

// Client saves messages
class VerifyClient : public TCPClient
{
public:
  VerifyClient(std::string const &host_name, std::string const &port_number,
               NetworkManager &nmanager)
    : TCPClient(nmanager)
  {
    Connect(host_name, port_number);
    this->OnMessage([](MessageBuffer const &value) {
      {
        FETCH_LOCK(mutex_);
        globalMessages.push_back(value);
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
std::vector<MessageBuffer> CreateTestData(std::size_t index)
{
  std::size_t messagesToSend = MID_CYCLES;
  globalMessages.clear();
  globalMessages.reserve(messagesToSend);
  bool smallPackets = true;

  std::vector<MessageBuffer> sendData;

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

    MessageBuffer arr;
    arr.Resize(packetSize);
    for (std::size_t z = 0; z < arr.size(); z++)
    {
      arr[z] = uint8_t(z + i);
    }
    sendData.push_back(arr);
  }

  return sendData;
}

TEST(tcp_client_stress_gtest, open_echo_server_multiple_times)
{
  std::cerr << "Info: Attempting to open the echo server multiple times" << std::endl;

  for (std::size_t index = 0; index < FEW_CYCLES; ++index)
  {
    GetOpenPort();
  }
}

TEST(tcp_client_stress_gtest, NM_dead)
{
  std::cerr << "Info: Attempting to open a connection to a port "
               "that doesn't exist (NM dead)"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager{"NetMgr", N};
    Client         client(host, std::to_string(emptyPort), nmanager);
  }
}

TEST(tcp_client_stress_gtest, NM_alive)
{
  std::cerr << "Info: Attempting to open a connection to a port "
               "that doesn't exist (NM alive)"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager{"NetMgr", N};
    nmanager.Start();
    Client client(host, std::to_string(emptyPort), nmanager);
    nmanager.Stop();
  }
}

TEST(tcp_client_stress_gtest, DISABLED_NM_jittering)
{
  std::cerr << "Info: Attempting to open a connection to a port that "
               "doesn't exist (NM jittering)"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  std::cerr << "starting" << std::endl;
  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager{"NetMgr", N};
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
}

TEST(tcp_client_stress_gtest, NM_dead_existing_port)
{
  std::cerr << "Info: Attempting to open a connection to a port that "
               "does exist (NM dead)"
            << std::endl;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager{"NetMgr", N};
    Client         client(host, port, nmanager);
  }
}

TEST(tcp_client_stress_gtest, NM_alive_existing_port)
{
  std::cerr << "Info: Attempting to open a connection to a "
               "port that does exist (NM alive)"
            << std::endl;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager{"NetMgr", N};
    nmanager.Start();
    Client client(host, port, nmanager);
    nmanager.Stop();
  }
}

TEST(tcp_client_stress_gtest, DISABLED_NM_jittering_existing_port)
{
  std::cerr << "Info: Attempting to open a connection to a "
               "port that does exist (NM jittering)"
            << std::endl;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));

  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    NetworkManager nmanager{"NetMgr", N};
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
}

TEST(tcp_client_stress_gtest, move_constr)
{
  std::cerr << "Info: Attempting to open multiple "
               "connections to a port that does exist (move constr)"
            << std::endl;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));
  std::vector<Client *>          clients;

  NetworkManager nmanager{"NetMgr", N};
  nmanager.Start();
  for (std::size_t index = 0; index < MANY_CYCLES; ++index)
  {
    Client client(host, port, nmanager);
    clients.push_back(&client);
  }
  nmanager.Stop();
}

TEST(tcp_client_stress_gtest, open_connection_to_existing_port_async)
{
  std::cerr << "Info: Attempting to open multiple "
               "connections to a port that does exist, async"
            << std::endl;
  std::string hostID                    = "localhost";
  uint16_t    portNo                    = 8080;
  std::string portNumberStr             = std::to_string(portNo);
  std::size_t networkManagerThreadCount = 1;

  // Start echo server
  fetch::network::LoopbackServer echo(uint16_t(std::stoi(port)));

  for (std::size_t index = 0; index < 10; ++index)
  {
    std::cerr << "Iteration: " << index << std::endl;
    NetworkManager nmanager{"NetMgr", networkManagerThreadCount};
    nmanager.Start();
    std::atomic<std::size_t> threadCount{0};
    std::size_t              iterations = MID_CYCLES;

    for (std::size_t i = 0; i < iterations; ++i)
    {
      std::thread([hostID, portNumberStr, nmanager, &threadCount] {
        NetworkManager managerCopy = nmanager;
        auto           i           = std::make_shared<Client>(hostID, portNumberStr, managerCopy);
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
}

TEST(tcp_client_stress_gtest, NM_being_destroyed_before_clients)
{
  std::cerr << "Info: (Legacy) Usually breaks due to the NM being destroyed "
               "before the clients"
            << std::endl;

  for (std::size_t index = 0; index < MID_CYCLES; ++index)
  {
    std::vector<Client *> clients;
    NetworkManager        nmanager{"NetMgr", N};
    nmanager.Start();

    for (std::size_t j = 0; j < 4; ++j)
    {
      Client client(host, port, nmanager);
      clients.push_back(&client);
    }

    nmanager.Stop();

    for (std::size_t j = 0; j < 4; ++j)
    {
      Client client(host, port, nmanager);
      clients.push_back(&client);
    }
    nmanager.Start();
    if ((index % 2) != 0u)
    {
      nmanager.Stop();
    }
    if ((index % 3) != 0u)
    {
      nmanager.Stop();
    }
    if ((index % 5) != 0u)
    {
      nmanager.Stop();
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
}

TEST(tcp_client_stress_gtest, bouncing_messages_off_server_and_counting)
{
  std::cerr << "Info: Bouncing messages off echo/loopback server and counting them" << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cerr << "Iteration: " << i << std::endl;
    fetch::network::LoopbackServer echoServer(emptyPort);
    NetworkManager                 nmanager{"NetMgr", N};
    nmanager.Start();
    Client client(host, std::to_string(emptyPort), nmanager);

    while (!client.is_alive())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::size_t currentCount   = clientReceivedCount;
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
    }

    nmanager.Stop();
  }
}

TEST(tcp_client_stress_gtest, bouncing_messages_off_server_and_counting_slow_clients)
{
  std::cerr << "Info: Bouncing messages off echo/loopback "
               "server and counting them, slow client "
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t i = 0; i < 5; ++i)
  {
    std::cerr << "Iteration: " << i << std::endl;
    fetch::network::LoopbackServer echoServer(emptyPort);
    NetworkManager                 nmanager{"NetMgr", N};
    nmanager.Start();
    SlowClient client(host, std::to_string(emptyPort), nmanager);

    while (!client.is_alive())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::size_t currentCount   = clientReceivedCount;
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
    }

    nmanager.Stop();
  }
}

TEST(tcp_client_stress_gtest, bouncing_messages_off_server_and_check_order)
{
  std::cerr << "Info: Bouncing messages off echo/loopback "
               "server and checking ordering"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cerr << "Iteration: " << i << std::endl;
    NetworkManager nmanager{"NetMgr", N};
    nmanager.Start();
    fetch::network::LoopbackServer echoServer(emptyPort);
    VerifyClient                   client(host, std::to_string(emptyPort), nmanager);

    while (!client.is_alive())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Precreate data, this handles clearing global counter etc.
    std::vector<MessageBuffer> sendData = CreateTestData(i);

    std::size_t expectCount = clientReceivedCount;

    for (auto &dat : sendData)
    {
      client.Send(dat);
      expectCount++;
    }

    while (clientReceivedCount != expectCount)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Verify we transmitted correctly
    EXPECT_NE(globalMessages.size(), 0) << "Failed to receive messages" << std::endl;

    EXPECT_EQ(globalMessages.size(), sendData.size())
        << "Failed to receive all messages" << std::endl;

    for (std::size_t j = 0; j < globalMessages.size(); ++j)
    {
      EXPECT_EQ(globalMessages[j], sendData[j]) << "Failed to verify messages" << std::endl;
    }
    nmanager.Stop();
  }
}

TEST(tcp_client_stress_gtest, bouncing_messages_off_server_and_check_order_multiple_clients)
{
  std::cerr << "Info: Bouncing messages off echo/loopback "
               "server and checking ordering, multiple clients"
            << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t index = 0; index < 10; ++index)
  {
    std::cerr << "Iteration: " << index << std::endl;
    fetch::network::LoopbackServer echoServer(emptyPort);
    NetworkManager                 nmanager{"NetMgr", N};
    nmanager.Start();
    std::vector<std::shared_ptr<VerifyClient>> clients;

    for (std::size_t i = 0; i < 5; ++i)
    {
      clients.push_back(std::make_shared<VerifyClient>(host, std::to_string(emptyPort), nmanager));
    }

    // Precreate data, this handles clearing global counter etc.
    std::vector<MessageBuffer> sendData = CreateTestData(index);

    for (auto const &i : clients)
    {
      while (!i->is_alive())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }

    std::size_t expectCount = clientReceivedCount;

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
    }

    // Verify we transmitted correctly
    EXPECT_NE(globalMessages.size(), 0) << "Failed to receive messages" << std::endl;

    EXPECT_EQ(globalMessages.size(), sendData.size())
        << "Failed to receive all messages" << std::endl;

    // We need to sort since this will not be ordered
    std::sort(globalMessages.begin(), globalMessages.end());
    std::sort(sendData.begin(), sendData.end());

    for (std::size_t i = 0; i < globalMessages.size(); ++i)
    {
      ASSERT_EQ(globalMessages[i], sendData[i]) << "Failed to verify messages" << std::endl;
    }

    nmanager.Stop();
  }
}

TEST(tcp_client_stress_gtest, DISABLED_killing_during_transmission)
{
  std::cerr << "Info: Killing during transmission, multiple clients" << std::endl;

  uint16_t emptyPort = GetOpenPort();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cerr << "Iteration: " << i << std::endl;
    fetch::network::LoopbackServer echoServer(emptyPort);
    NetworkManager                 nmanager{"NetMgr", N};
    nmanager.Start();
    std::vector<std::shared_ptr<VerifyClient>> clients;

    for (std::size_t j = 0; j < 5; ++j)
    {
      clients.push_back(std::make_shared<VerifyClient>(host, std::to_string(emptyPort), nmanager));
    }

    std::size_t messagesToSend = MID_CYCLES;
    globalMessages.clear();
    globalMessages.reserve(messagesToSend);

    for (auto const &client : clients)
    {
      while (!client->is_alive())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }

    // Precreate data
    std::vector<MessageBuffer> sendData;

    for (uint8_t k = 0; k < 8; k++)
    {
      std::size_t   packetSize = 1000;
      MessageBuffer arr;
      arr.Resize(packetSize);
      for (std::size_t z = 0; z < arr.size(); z++)
      {
        arr[z] = k;
      }

      sendData.push_back(arr);
    }

    for (std::size_t j = 0; j < messagesToSend; ++j)
    {
      for (auto const &msg : sendData)
      {
        for (auto const &client : clients)
        {
          std::thread([client, msg]() { client->Send(msg); }).detach();
        }
      }
    }
    if ((i % 2) != 0u)
    {
      nmanager.Stop();
    }
  }
}

}  // namespace
