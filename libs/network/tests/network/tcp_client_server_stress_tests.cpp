//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
#include <gtest/gtest.h>
#include <iostream>
#include <memory>

#include "core/byte_array/encoders.hpp"
#include "network/tcp/tcp_client.hpp"
#include "network/tcp/tcp_server.hpp"

// Test of the client and server.

using namespace fetch::network;
using namespace fetch::byte_array;

static constexpr char const *LOGGING_NAME = "TcpClientServerStressTests";

std::atomic<std::size_t> clientReceivedCount_{0};
std::atomic<std::size_t> serverReceivedCount{0};

std::vector<message_type> globalMessagesFromServer_{};
std::vector<message_type> globalMessagesToServer_{};

// Basic server
class Server : public TCPServer
{
public:
  Server(uint16_t port, NetworkManager nmanager)
    : TCPServer(port, nmanager)
  {
    Start();
  }
  ~Server() = default;

  void PushRequest(connection_handle_type /*client*/, message_type const &msg) override
  {
    std::cerr << "Message: " << msg << std::endl;
  }
};

// Basic client increments on received messages
class Client : public TCPClient
{
public:
  Client(std::string const &host, uint16_t &port, NetworkManager &nmanager)
    : TCPClient(nmanager)
  {
    Connect(host, port);
  }

  ~Client()
  {
    TCPClient::Cleanup();
  }
};

void waitUntilConnected(std::string const &host, uint16_t port)
{
  static NetworkManager nmanager(1);
  nmanager.Start();

  while (true)
  {
    Client client(host, port, nmanager);

    for (std::size_t i = 0; i < 4; ++i)
    {
      if (client.is_alive())
      {
        return;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

template <std::size_t N = 1>
void TestCase0(std::string /*host*/, uint16_t port)
{
  std::cerr << "\nTEST CASE 0. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open the server multiple times" << std::endl;

  for (std::size_t index = 0; index < 20; ++index)
  {
    NetworkManager nmanager(N);
    Server         server(port, nmanager);
    nmanager.Start();
  }

  SUCCEED() << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase1(std::string /*host*/, uint16_t port)
{
  std::cerr << "\nTEST CASE 1. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open the server multiple times" << std::endl;

  for (std::size_t index = 0; index < 20; ++index)
  {
    NetworkManager nmanager(N);
    if (index % 2)
    {
      nmanager.Start();
    }
    Server server(port, nmanager);
    if (index % 3)
    {
      nmanager.Stop();
    }
    nmanager.Start();
  }

  SUCCEED() << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase2(std::string host, uint16_t port)
{
  std::cerr << "\nTEST CASE 2. Threads: " << N << std::endl;
  std::cerr << "Info: Attempting to open the server and send data to it" << std::endl;

  for (std::size_t index = 0; index < 20; ++index)
  {
    NetworkManager nmanager(N);
    nmanager.Start();
    Server server(port, nmanager);
    waitUntilConnected(host, port);

    Client client(host, port, nmanager);

    while (!client.is_alive())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }
    client.Send("test this");
    if (index % 3)
    {
      nmanager.Stop();
    }
  }

  SUCCEED() << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase3(std::string host, uint16_t port)
{
  std::cerr << "\nTEST CASE 3. Threads: " << N << std::endl;
  std::cerr << "Info: Destruct server while people are connecting to it " << std::endl;

  for (std::size_t index = 0; index < 3; ++index)
  {
    NetworkManager nmanager(N);
    nmanager.Start();
    std::unique_ptr<Server> server = std::make_unique<Server>(port, nmanager);

    waitUntilConnected(host, port);
    std::atomic<std::size_t> threadCount{0};
    std::size_t              iterations = 100;

    for (std::size_t i = 0; i < iterations; ++i)
    {
      std::thread([&host, &port, nmanager, &threadCount] {
        NetworkManager managerCopy = nmanager;
        auto           i           = std::make_shared<Client>(host, port, managerCopy);
        i->Send("test");
        threadCount++;
      })
          .detach();
    }

    server.reset();

    while (threadCount != iterations)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }

    if (index % 3)
    {
      nmanager.Stop();
    }
  }

  SUCCEED() << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase4(std::string host, uint16_t port)
{
  std::cerr << "\nTEST CASE 4. Threads: " << N << std::endl;
  std::cerr << "Info: Destruct server, test that its acceptor is dying " << std::endl;

  NetworkManager nmanager(N);
  nmanager.Start();

  for (std::size_t index = 0; index < 3; ++index)
  {
    std::unique_ptr<Server> server = std::make_unique<Server>(port, nmanager);

    waitUntilConnected(host, port);
    std::atomic<std::size_t> threadCount{0};
    std::size_t              iterations = 100;

    for (std::size_t i = 0; i < iterations; ++i)
    {
      std::thread([&host, &port, nmanager, &threadCount] {
        NetworkManager managerCopy = nmanager;
        auto           i           = std::make_shared<Client>(host, port, managerCopy);
        i->Send("test");
        threadCount++;
      })
          .detach();
    }

    if (index % 2)
    {
      server.reset();
    }

    while (threadCount != iterations)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }
  }

  SUCCEED() << "Success." << std::endl;
}
class TCPClientServerTest : public testing::TestWithParam<std::size_t>
{
};

TEST_P(TCPClientServerTest, basic_test)
{

  std::string host       = "localhost";
  uint16_t    portNumber = 8079;

  std::cerr << "Testing communications on port: " << portNumber << std::endl;

  std::size_t iterations = GetParam();

  FETCH_LOG_INFO(LOGGING_NAME, "Running test iterations: ", iterations);

  for (std::size_t i = 0; i < iterations; ++i)
  {
    TestCase0<1>(host, portNumber);
    TestCase1<1>(host, portNumber);
    TestCase2<1>(host, portNumber);
    TestCase3<1>(host, portNumber);
    TestCase4<1>(host, portNumber);

    TestCase0<10>(host, portNumber);
    TestCase1<10>(host, portNumber);
    TestCase2<10>(host, portNumber);
    TestCase3<10>(host, portNumber);
    TestCase4<10>(host, portNumber);
  }

  SUCCEED() << "Success." << std::endl;
}
INSTANTIATE_TEST_CASE_P(MyGroup, TCPClientServerTest, testing::Values<std::size_t>(4),
                        testing::PrintToStringParamName());
// testing::Values<std::size_t>(4): 4 is the number of iterations to run this test under 30 sec.
// Change the number of iteration to increase/decrease test execution time.