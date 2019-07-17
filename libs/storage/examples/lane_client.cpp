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

#include "core/byte_array/consumers.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "core/string/trim.hpp"
#include "ledger/chain/transaction.hpp"
#include "network/service/service_client.hpp"
#include "storage/document_store_protocol.hpp"
#include "version/cli_header.hpp"

#include <iostream>

using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;

class MultiLaneDBClient  //: private
{
public:
  using client_type        = ServiceClient<fetch::network::TCPClient>;
  using shared_client_type = std::shared_ptr<client_type>;

  MultiLaneDBClient(uint32_t lanes, std::string const &host, uint16_t port,
                    fetch::network::NetworkManager &tm)
  {
    id_ = "my-fetch-id";
    for (uint32_t i = 0; i < lanes; ++i)
    {
      lanes_.push_back(std::make_shared<client_type>(host, uint16_t(port + i), tm));
    }
  }

  ByteArray Get(ByteArray const &key)
  {
    auto        res  = fetch::storage::ResourceAddress(key);
    std::size_t lane = res.lane(uint32_t(lanes_.size()));
    //    std::cout << "Getting " << key << " from lane " << lane <<  " " <<
    //    byte_array::ToBase64(res.id()) << std::endl;
    auto promise = lanes_[lane]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::GET, res);

    return promise.As<storage::Document>().document;
  }

  bool Lock(ByteArray const &key)
  {
    //    std::cout << "Locking: " << key << std::endl;

    auto        res  = fetch::storage::ResourceAddress(key);
    std::size_t lane = res.lane(uint32_t(lanes_.size()));
    auto        promise =
        lanes_[lane]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::LOCK, res);

    return promise.As<bool>();
  }

  bool Unlock(ByteArray const &key)
  {
    //    std::cout << "Unlocking: " << key << std::endl;

    auto        res  = fetch::storage::ResourceAddress(key);
    std::size_t lane = res.lane(uint32_t(lanes_.size()));
    auto        promise =
        lanes_[lane]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK, res);

    return promise.As<bool>();
  }

  void Set(ByteArray const &key, ByteArray const &value)
  {
    auto        res  = fetch::storage::ResourceAddress(key);
    std::size_t lane = res.lane(uint32_t(lanes_.size()));
    //    std::cout << "Setting " << key <<  " on lane " << lane << " " <<
    //    byte_array::ToBase64(res.id()) << std::endl;
    auto promise =
        lanes_[lane]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::SET, res, value);
    promise.Wait(2000);
  }

  void Commit(uint64_t bookmark)
  {
    std::vector<service::Promise> promises;
    for (std::size_t i = 0; i < lanes_.size(); ++i)
    {
      auto promise =
          lanes_[i]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::COMMIT, bookmark);
      promises.push_back(promise);
    }

    for (auto &p : promises)
    {
      p.Wait();
    }
  }

  void Revert(uint64_t bookmark)
  {
    std::vector<service::Promise> promises;
    for (std::size_t i = 0; i < lanes_.size(); ++i)
    {
      auto promise =
          lanes_[i]->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::REVERT, bookmark);
      promises.push_back(promise);
    }

    for (auto &p : promises)
    {
      p.Wait();
    }
  }

  ByteArray Hash()
  {
    return lanes_[0]
        ->Call(0, fetch::storage::RevertibleDocumentStoreProtocol::HASH)
        .As<ByteArray>();
  }

  void SetID(ByteArray const &id)
  {
    id_ = id;
  }

  void AddTransaction(ConstByteArray const &tx_data)
  {
    json::JSONDocument  doc(tx_data);
    ledger::Transaction tx;
  }

  void AddTransaction(ledger::Transaction &tx)
  {
    //    tx.UpdateDigests();
  }

  ByteArray const &id()
  {
    return id_;
  }

private:
  ByteArray                       id_;
  std::vector<shared_client_type> lanes_;
};

enum
{
  TOKEN_NAME      = 1,
  TOKEN_STRING    = 2,
  TOKEN_NUMBER    = 3,
  TOKEN_CATCH_ALL = 12
};

void AddTransactionDialog()
{
  ledger::Transaction tx;
  std::string         contract_name, args, res;
  std::cout << "Contract name: ";

  std::getline(std::cin, contract_name);
  fetch::string::Trim(contract_name);
  tx.set_contract_name(contract_name);

  std::cout << "Arguments: ";
  std::getline(std::cin, args);
  fetch::string::Trim(args);
  tx.set_arguments(args);

  std::cout << "Resources: " << std::endl;

  std::getline(std::cin, res);
  fetch::string::Trim(res);
  while (res != "")
  {
    tx.PushGroup(res);
    std::getline(std::cin, res);
    fetch::string::Trim(res);
  }
  double fee;
  std::cout << "Gas: ";
  std::cin >> fee;
}

int main(int argc, char const **argv)
{
  fetch::logger.DisableLogger();
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint32_t lane_count = params.GetParam<uint32_t>("lane-count", 1);

  std::cout << std::endl;
  fetch::version::DisplayCLIHeader("Multi-lane client");
  std::cout << "Connecting with " << lane_count << " lanes." << std::endl;

  // Client setup
  fetch::network::NetworkManager tm(8);
  MultiLaneDBClient              client(lane_count, "localhost", 8080, tm);

  tm.Start();

  std::string line = "";

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
        if (command[0] == "addtx")
        {
          if (command.size() == 1)
          {
            AddTransactionDialog();
          }
          else
          {
            std::cout << "usage: add" << std::endl;
          }
        }
        else if (command[0] == "get")
        {
          if (command.size() == 2)
          {
            std::cout << client.Get(command[1]) << std::endl;
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
            client.Lock(command[1]);
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
            client.Unlock(command[1]);
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
            client.Set(command[1], command[2]);
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
    catch (serializers::SerializableException const &e)
    {
      std::cerr << "error: " << e.what() << std::endl;
    }
  }

  tm.Stop();

  return 0;
}
