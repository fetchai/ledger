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

#define FETCH_DISABLE_TODO_COUT
#include <functional>
#include <iostream>

#include "core/commandline/parameter_parser.hpp"

#include "network/protocols.hpp"
#include "network/service/client.hpp"
#include "network/service/server.hpp"
#include "network/tcp/tcp_server.hpp"

#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"
#include "http/server.hpp"

#include "core/byte_array/encoders.hpp"
#include "core/random/lfg.hpp"
#include "crypto/hash.hpp"
#include "testing/unittest.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

using namespace fetch::protocols;
using namespace fetch::commandline;
using namespace fetch::byte_array;

static constexpr char const *LOGGING_NAME = "node";

using tx_type = typename ShardManager::transaction_type;

std::vector<std::string> words = {
    "squeak",     "fork",        "governor",  "peace",   "courageous", "support",   "tight",
    "reject",     "extra-small", "slimy",     "form",    "bushes",     "telling",   "outrageous",
    "cure",       "occur",       "plausible", "scent",   "kick",       "melted",    "perform",
    "rhetorical", "good",        "selfish",   "dime",    "tree",       "prevent",   "camera",
    "paltry",     "allow",       "follow",    "balance", "wave",       "curved",    "woman",
    "rampant",    "eatable",     "faulty",    "sordid",  "tooth",      "bitter",    "library",
    "spiders",    "mysterious",  "stop",      "talk",    "watch",      "muddle",    "windy",
    "meal",       "arm",         "hammer",    "purple",  "company",    "political", "territory",
    "open",       "attract",     "admire",    "undress", "accidental", "happy",     "lock",
    "delicious"};

fetch::random::LaggedFibonacciGenerator<> lfg;
tx_type                                   RandomTX(std::size_t const &n)
{

  std::string ret = words[lfg() & 63];
  for (std::size_t i = 1; i < n; ++i)
  {
    ret += " " + words[lfg() & 63];
  }
  tx_type t;
  t.set_body(ret);
  return t;
}

class FetchShardService
{
public:
  FetchShardService(uint16_t port)
    : network_manager_(new fetch::network::NetworkManager(8))
    , service_(port, network_manager_)
    , http_server_(8080, network_manager_)
  {

    std::cout << "Listening for peers on " << (port) << ", clients on " << (port) << std::endl;

    // Creating a service contiaing the shard protocol
    shard_ = new ShardProtocol(network_manager_, FetchProtocols::SHARD);
    service_.Add(FetchProtocols::SHARD, shard_);

    // Creating a http server based on the shard protocol
    http_server_.AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    http_server_.AddMiddleware(fetch::http::middleware::ColorLog);
    http_server_.AddModule(*shard_);
  }

  ~FetchShardService()
  {
    std::cout << "Killing fetch service";
    delete shard_;
  }

  void Start() { network_manager_->Start(); }

  void Stop() { network_manager_->Stop(); }

private:
  fetch::network::NetworkManager *                         network_manager_;
  fetch::service::ServiceServer<fetch::network::TCPServer> service_;
  fetch::http::HTTPServer                                  http_server_;

  ShardProtocol *shard_ = nullptr;
};

FetchShardService *service;
void               StartService(int port)
{
  service = new FetchShardService(port);
  service->Start();
}

int main(int argc, char const **argv)
{
  StartService(1337);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  using client_type            = fetch::service::ServiceClient<fetch::network::TCPClient>;
  using client_shared_ptr_type = std::shared_ptr<client_type>;

  fetch::network::NetworkManager tm(2);
  client_shared_ptr_type         client = std::make_shared<client_type>("localhost", 1337, &tm);
  tm.Start();

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  auto ping_promise = client->Call(FetchProtocols::SHARD, ShardRPC::PING);
  if (!ping_promise.Wait(2000))
  {
    FETCH_LOG_ERROR(LOGGING_NAME,"Client not repsonding - hanging up!");
    exit(-1);
  }

  ShardManager manager;

  for (std::size_t i = 0; i < 100; ++i)
  {

    auto tx = RandomTX(4);
    std::cout << tx.body() << std::endl;

    client->Call(FetchProtocols::SHARD, ShardRPC::PUSH_TRANSACTION, tx);
    manager.PushTransaction(tx);

    auto server_block = client->Call(FetchProtocols::SHARD, ShardRPC::GET_NEXT_BLOCK)
                            .As<ShardManager::block_type>();
    auto local_block = manager.GetNextBlock();

    if ((server_block.body().previous_hash != local_block.body().previous_hash) ||
        (server_block.header() != local_block.header()) ||
        (server_block.body().transaction_hash != local_block.body().transaction_hash))
    {
      std::cout << "FAILED" << std::endl;

      std::cout << "Server block: " << server_block.meta_data().block_number << " "
                << server_block.meta_data().total_work << std::endl;
      std::cout << "  <- " << fetch::byte_array::ToBase64(server_block.body().previous_hash)
                << std::endl;
      std::cout << "   = " << fetch::byte_array::ToBase64(server_block.header()) << std::endl;
      std::cout << "    (" << fetch::byte_array::ToBase64(server_block.body().transaction_hash)
                << ")" << std::endl;
      std::cout << "Local block: " << local_block.meta_data().block_number << " "
                << local_block.meta_data().total_work << std::endl;
      std::cout << "  <- " << fetch::byte_array::ToBase64(local_block.body().previous_hash)
                << std::endl;
      std::cout << "   = " << fetch::byte_array::ToBase64(local_block.header()) << std::endl;
      std::cout << "    (" << fetch::byte_array::ToBase64(local_block.body().transaction_hash)
                << ")" << std::endl;
      exit(-1);
    }

    auto &p1 = server_block.proof();
    p1.SetTarget(lfg() % 5);
    while (!p1()) ++p1;

    auto &p2 = local_block.proof();
    p2.SetTarget(lfg() % 5);
    while (!p2()) ++p2;

    client->Call(FetchProtocols::SHARD, ShardRPC::PUSH_BLOCK, server_block);
    client->Call(FetchProtocols::SHARD, ShardRPC::PUSH_BLOCK, local_block);

    manager.PushBlock(local_block);
    manager.PushBlock(server_block);

    client->Call(FetchProtocols::SHARD, ShardRPC::COMMIT);
    manager.Commit();
  }

  tm.Stop();

  service->Stop();
  delete service;

  return 0;
}
