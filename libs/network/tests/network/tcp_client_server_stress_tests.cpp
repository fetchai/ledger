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

std::vector<message_type> globalMessagesFromServer_{};
std::mutex                messages_;

// Basic server
class Server : public TCPServer
{
public:
  Server(uint16_t port, NetworkManager nmanager)
    : TCPServer(port, nmanager)
  {}  // note for debug purposes, server does not Start() automatically

  ~Server() override = default;

  void PushRequest(connection_handle_type /*client*/, message_type const &msg) override
  {
    /*std::cerr << "Message: " << msg << std::endl;*/
    std::lock_guard<std::mutex> lock(messages_);
    globalMessagesFromServer_.push_back(msg);
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
  static NetworkManager nmanager{"NetMgr", 1};
  nmanager.Start();
  int attempts = 0;

  while (true)
  {
    Client client(host, port, nmanager);

    for (std::size_t i = 0; i < 4; ++i)
    {
      if (client.WaitForAlive(10))
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Connected successfully to ", host, ":", port);
        return;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (attempts++ % 10 == 0)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Waiting for client to connect to: ", port);
    }

    if (attempts == 50)
    {
      throw std::runtime_error("Failed to connect test client to port");
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
    NetworkManager nmanager{"NetMgr", N};

    // Delay network manager starting arbitrarily
    auto cb = [&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      nmanager.Start();
    };
    auto dummy = std::async(std::launch::async, cb);  // dummy is important to force async execution

    Server server(port, nmanager);
    server.Start();
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
    NetworkManager nmanager{"NetMgr", N};
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
    server.Start();
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
    NetworkManager nmanager{"NetMgr", N};
    nmanager.Start();

    Server server(port, nmanager);

    auto cb    = [&] { server.Start(); };
    auto dummy = std::async(std::launch::async, cb);  // dummy is important to force async execution

    waitUntilConnected(host, port);

    Client client(host, port, nmanager);

    while (!client.is_alive())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(4));
      FETCH_LOG_INFO(LOGGING_NAME, "Waiting for client to connect");
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
    NetworkManager nmanager{"NetMgr", N};
    nmanager.Start();
    std::unique_ptr<Server> server = std::make_unique<Server>(port, nmanager);
    server->Start();

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
      }).detach();
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

  NetworkManager nmanager{"NetMgr", N};
  nmanager.Start();

  for (std::size_t index = 0; index < 3; ++index)
  {
    std::unique_ptr<Server> server = std::make_unique<Server>(port, nmanager);
    server->Start();

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
      }).detach();
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

template <std::size_t N = 1>
void TestCase5(std::string host, uint16_t port)
{
  std::cerr << "\nTEST CASE 5. Threads: " << N << std::endl;
  std::cerr << "Verify very large packet transmission, client side" << std::endl;

  NetworkManager nmanager{"NetMgr", N};
  nmanager.Start();

  for (std::size_t index = 0; index < 3; ++index)
  {
    std::unique_ptr<Server> server = std::make_unique<Server>(port, nmanager);
    server->Start();

    waitUntilConnected(host, port);

    // Create packets of varying sizes
    std::vector<message_type> to_send;

    {
      std::lock_guard<std::mutex> lock(messages_);
      globalMessagesFromServer_.clear();
    }

    for (std::size_t i = 0; i < 5; ++i)
    {
      char to_fill = char(0x41 + (i & 0xFF));  // 0x41 = 'A'

      std::string send_me(1 << (i + 14), to_fill);

      to_send.push_back(send_me);
    }

    // This will be over a single connection
    auto i = std::make_shared<Client>(host, port, nmanager);
    if (!(i->WaitForAlive(100)))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Client never opened");
      throw 1;
    }

    std::atomic<std::size_t> send_atomic{0};

    auto cb = [&] {
      std::size_t send_index = send_atomic++;

      if (send_index < to_send.size())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Sending ", send_index);
        i->Send(to_send[send_index]);
      }
    };

    for (std::size_t i = 0; i < to_send.size(); ++i)
    {
      auto dummy0 =
          std::async(std::launch::async, cb);  // dummy is important to force async execution
      auto dummy1 = std::async(std::launch::async, cb);
      auto dummy2 = std::async(std::launch::async, cb);
      auto dummy3 = std::async(std::launch::async, cb);
      auto dummy4 = std::async(std::launch::async, cb);
    }

    for (;;)
    {
      {
        std::lock_guard<std::mutex> lock(messages_);

        if (globalMessagesFromServer_.size() == to_send.size())
        {
          break;
        }
      }

      FETCH_LOG_DEBUG(LOGGING_NAME, "Waiting for messages to arrive");
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::lock_guard<std::mutex> lock(messages_);
    std::sort(globalMessagesFromServer_.begin(), globalMessagesFromServer_.end());
    std::sort(to_send.begin(), to_send.end());

    if (globalMessagesFromServer_ != to_send)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to match server messages. Recieved: ");

      for (auto const &i : globalMessagesFromServer_)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, i);
      }
    }
  }

  SUCCEED() << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase6(std::string host, uint16_t port)
{
  std::cerr << "\nTEST CASE 6. Threads: " << N << std::endl;
  std::cerr << "Verify very large packet transmission, tcp server side" << std::endl;

  NetworkManager nmanager{"NetMgr", N};
  nmanager.Start();

  for (std::size_t index = 0; index < 3; ++index)
  {
    std::unique_ptr<Server> server = std::make_unique<Server>(port, nmanager);
    server->Start();

    waitUntilConnected(host, port);

    // Create packets of varying sizes
    std::vector<message_type> to_send;
    std::vector<message_type> to_recieve;

    for (std::size_t i = 0; i < 5; ++i)
    {
      char to_fill = char(0x41 + (i & 0xFF));  // 0x41 = 'A'

      std::string send_me(1 << (i + 14), to_fill);
      to_send.push_back(send_me);
    }

    FETCH_LOG_INFO(LOGGING_NAME, "*** Open connection. ***");
    // This will be over a single connection
    auto i = std::make_shared<Client>(host, port, nmanager);
    if (!(i->WaitForAlive(1000)))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Client never opened 1");
      throw 1;
    }

    std::mutex messages_recieve;

    auto lam = [&](message_type const &msg) {
      std::lock_guard<std::mutex> lock(messages_recieve);
      to_recieve.push_back(msg);
    };

    i->OnMessage(lam);

    std::atomic<std::size_t> send_atomic{0};

    auto cb = [&] {
      std::size_t send_index = send_atomic++;

      if (send_index < to_send.size())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Sending ", send_index);
        server->Broadcast(to_send[send_index]);
      }
    };

    for (std::size_t i = 0; i < to_send.size(); ++i)
    {
      auto dummy0 =
          std::async(std::launch::async, cb);  // dummy is important to force async execution
      auto dummy1 = std::async(std::launch::async, cb);
      auto dummy2 = std::async(std::launch::async, cb);
      auto dummy3 = std::async(std::launch::async, cb);
      auto dummy4 = std::async(std::launch::async, cb);
    }

    for (;;)
    {
      {
        std::lock_guard<std::mutex> lock(messages_recieve);

        if (to_recieve.size() == to_send.size())
        {
          break;
        }
      }

      FETCH_LOG_DEBUG(LOGGING_NAME, "Waiting for messages to arrive");
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::sort(to_recieve.begin(), to_recieve.end());
    std::sort(to_send.begin(), to_send.end());

    if (to_recieve != to_send)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to match server messages. Recieved: ");

      for (auto const &i : globalMessagesFromServer_)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, i);
      }
    }
  }

  SUCCEED() << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase7(std::string host, uint16_t port)
{
  std::cerr << "\nTEST CASE 7. Threads: " << N << std::endl;
  std::cerr << "Verify very large packet transmission, bidirectional at once" << std::endl;

  NetworkManager nmanager{"NetMgr", N};
  nmanager.Start();

  for (std::size_t index = 0; index < 3; ++index)
  {
    std::unique_ptr<Server> server = std::make_unique<Server>(port, nmanager);
    server->Start();

    waitUntilConnected(host, port);

    // Create packets of varying sizes
    std::vector<message_type> to_send_from_server;
    std::vector<message_type> to_send_from_client;

    std::vector<message_type> recieved_client;

    {
      std::lock_guard<std::mutex> lock(messages_);
      globalMessagesFromServer_.clear();
    }

    for (std::size_t i = 0; i < 5; ++i)
    {
      char to_fill = char(0x41 + (i & 0xFF));  // 0x41 = 'A'

      std::string send_me(1 << (i + 14), to_fill);
      to_send_from_client.push_back(send_me);
    }

    for (std::size_t i = 0; i < 5; ++i)
    {
      char to_fill = char(0x49 + (i & 0xFF));

      std::string send_me(1 << (i + 14), to_fill);
      to_send_from_server.push_back(send_me);
    }

    FETCH_LOG_INFO(LOGGING_NAME, "*** Open connection. ***");
    // This will be over a single connection
    auto i = std::make_shared<Client>(host, port, nmanager);
    if (!(i->WaitForAlive(1000)))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Client never opened!");
      throw 1;
    }

    std::mutex messages_recieve;

    auto lam = [&](message_type const &msg) {
      std::lock_guard<std::mutex> lock(messages_recieve);
      recieved_client.push_back(msg);
    };

    i->OnMessage(lam);

    std::atomic<std::size_t> send_atomic_client{0};
    std::atomic<std::size_t> send_atomic_server{0};

    auto cb_server = [&] {
      std::size_t send_index = send_atomic_client++;

      if (send_index < to_send_from_server.size())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Sending from server", send_index);
        server->Broadcast(to_send_from_server[send_index]);
      }
    };

    auto cb_client = [&] {
      std::size_t send_index = send_atomic_server++;

      if (send_index < to_send_from_client.size())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Sending from client", send_index);
        i->Send(to_send_from_client[send_index]);
      }
    };

    for (std::size_t i = 0; i < to_send_from_client.size(); ++i)
    {
      auto dummy0 = std::async(std::launch::async, cb_server);
      auto dummy1 = std::async(std::launch::async, cb_client);
      auto dummy2 = std::async(std::launch::async, cb_server);
      auto dummy3 = std::async(std::launch::async, cb_client);
    }

    for (;;)
    {
      {
        std::lock_guard<std::mutex> lock(messages_recieve);

        if (recieved_client.size() == to_send_from_server.size() &&
            globalMessagesFromServer_.size() == to_send_from_client.size())
        {
          break;
        }
      }

      FETCH_LOG_DEBUG(LOGGING_NAME, "Waiting for messages to arrive");
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::lock_guard<std::mutex> lock(messages_);
    std::sort(globalMessagesFromServer_.begin(), globalMessagesFromServer_.end());
    std::sort(to_send_from_client.begin(), to_send_from_client.end());

    std::sort(recieved_client.begin(), recieved_client.end());
    std::sort(to_send_from_server.begin(), to_send_from_server.end());

    if (globalMessagesFromServer_ != to_send_from_client)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to match client->server messages.");
      throw 1;
    }

    if (recieved_client != to_send_from_server)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to match server->client messages.");
      throw 1;
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
    TestCase5<1>(host, portNumber);
    TestCase6<1>(host, portNumber);
    TestCase7<1>(host, portNumber);

    TestCase0<10>(host, portNumber);
    TestCase1<10>(host, portNumber);
    TestCase2<10>(host, portNumber);
    TestCase3<10>(host, portNumber);
    TestCase4<10>(host, portNumber);
    TestCase5<10>(host, portNumber);
    TestCase6<10>(host, portNumber);
    TestCase7<10>(host, portNumber);
  }

  SUCCEED() << "Success." << std::endl;
}
INSTANTIATE_TEST_CASE_P(MyGroup, TCPClientServerTest, testing::Values<std::size_t>(4),
                        testing::PrintToStringParamName());
// testing::Values<std::size_t>(4): 4 is the number of iterations to run this test under 30 sec.
// Change the number of iteration to increase/decrease test execution time.
