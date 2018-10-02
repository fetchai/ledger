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

#include "common.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "core/string/trim.hpp"
#include "ledger/chain/helper_functions.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "network/service/service_client.hpp"
#include "network/tcp/tcp_client.hpp"
#include "storage/document_store_protocol.hpp"

#include <iostream>
using namespace fetch;

using namespace fetch::ledger;
using namespace fetch::byte_array;

using LaneIndex = fetch::ledger::StorageUnitClient::LaneIndex;

enum
{
  TOKEN_NAME      = 1,
  TOKEN_STRING    = 2,
  TOKEN_NUMBER    = 3,
  TOKEN_CATCH_ALL = 12
};

using ResourceAddress = fetch::storage::ResourceAddress;
using TCPClient       = fetch::network::TCPClient;

static constexpr char const *LOGGING_NAME = "lane_client.cpp";

int main(int argc, char **argv)
{
  // Parameters
  fetch::logger.DisableLogger();
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint32_t num_lanes       = params.GetParam<uint32_t>("lane-count", 1);
  uint16_t lane_port_start = 8080;

  std::cout << std::endl;
  fetch::commandline::DisplayCLIHeader("Storage Unit Client");
  std::cout << "Connecting with " << num_lanes << " lanes." << std::endl;

  // Client setup
  fetch::network::NetworkManager tm(8);
  StorageUnitClient              client(tm);

  tm.Start();

  std::map<LaneIndex, std::pair<byte_array::ByteArray, uint16_t>> lane_data;
  for (LaneIndex i = 0; i < num_lanes; ++i)
  {
    uint16_t const lane_port = static_cast<uint16_t>(lane_port_start + i);
    lane_data[i]             = std::make_pair(byte_array::ByteArray("127.0.0.1"), lane_port);
  }

  auto count =
      client.AddLaneConnectionsWaiting<TCPClient>(lane_data, std::chrono::milliseconds(30000));
  if (count != num_lanes)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Lane connections NOT established.");
    exit(1);
  }

  // Taking commands
  std::string            line = "";
  Tokenizer              tokenizer;
  std::vector<ByteArray> command;

  tokenizer.AddConsumer(consumers::StringConsumer<TOKEN_STRING>);
  tokenizer.AddConsumer(consumers::NumberConsumer<TOKEN_NUMBER>);
  tokenizer.AddConsumer(consumers::Token<TOKEN_NAME>);
  tokenizer.AddConsumer(consumers::AnyChar<TOKEN_CATCH_ALL>);

  while ((std::cin) && (line != "quit"))
  {
    std::cout << ">> ";

    // Getting command
    std::getline(std::cin, line);
    try
    {

      command.clear();
      tokenizer.clear();
      tokenizer.Parse(line);

      for (auto &t : tokenizer)
      {
        if (t.type() != TOKEN_CATCH_ALL)
        {
          command.push_back(t);
        }
      }

      if (command.size() > 0)
      {
        if (command[0] == "gettx")
        {
          if (command.size() != 2)
          {
            std::cout << "usage: gettx \"[hash]\"" << std::endl;
          }
          else
          {
            auto enckey = command[1].SubArray(1, command[1].size() - 2);
            auto key    = byte_array::FromBase64(enckey);

            chain::Transaction tx;
            client.GetTransaction(key, tx);
            std::cout << std::endl;

            std::cout << "Transaction: " << byte_array::ToBase64(tx.digest()) << std::endl;
            examples::common::ToBase64(std::cout, tx.signatures());
            std::cout << "Fee: " << tx.summary().fee << std::endl;
            std::cout << std::endl;
          }
        }
        else if (command[0] == "addtx")
        {
          if (command.size() == 1)
          {
            chain::Transaction tx = chain::VerifiedTransaction::Create(chain::RandomTransaction());
            std::cout << std::endl;
            std::cout << "Transaction: " << byte_array::ToBase64(tx.digest()) << std::endl;
            examples::common::ToBase64(std::cout, tx.signatures());
            std::cout << "Fee: " << tx.summary().fee << std::endl;
            std::cout << std::endl;

            client.AddTransaction(tx);
          }
          else
          {
            std::cout << "usage: addtx" << std::endl;
          }
        }
        else if (command[0] == "get")
        {
          if (command.size() == 2)
          {
            std::cout << client.Get(ResourceAddress{command[1]}).document << std::endl;
          }
          else
          {
            std::cout << "usage: get [id]" << std::endl;
          }
        }
        else if (command[0] == "lock")
        {

          if (command.size() == 2)
          {
            client.Lock(ResourceAddress{command[1]});
          }
          else
          {
            std::cout << "usage: lock [id]" << std::endl;
          }
        }
        else if (command[0] == "unlock")
        {
          if (command.size() == 2)
          {
            client.Unlock(ResourceAddress{command[1]});
          }
          else
          {
            std::cout << "usage: unlock [id]" << std::endl;
          }
        }
        else if (command[0] == "set")
        {
          if (command.size() == 3)
          {
            client.Set(ResourceAddress{command[1]}, command[2]);
          }
          else
          {
            std::cout << "usage: set [id] \"[value]\"" << std::endl;
          }
        }
        else if (command[0] == "commit")
        {
          if (command.size() == 2)
          {
            uint64_t bookmark = uint64_t(command[1].AsInt());
            client.Commit(bookmark);
          }
          else
          {
            std::cout << "usage: commit [bookmark,int]" << std::endl;
          }
        }
        else if (command[0] == "revert")
        {
          if (command.size() == 2)
          {
            uint64_t bookmark = uint64_t(command[1].AsInt());
            client.Revert(bookmark);
          }
          else
          {
            std::cout << "usage: revert [bookmark,int]" << std::endl;
          }
        }
        else if (command[0] == "hash")
        {
          if (command.size() == 1)
          {
            ByteArray barr = client.Hash();
            std::cout << "State hash: " << ToBase64(barr) << std::endl;
          }
          else
          {
            std::cout << "usage: set [id] \"[value]\"" << std::endl;
          }
        }
      }
    }
    catch (serializers::SerializableException &e)
    {
      std::cerr << "error: " << e.what() << std::endl;
    }
  }

  tm.Stop();

  return 0;
}
