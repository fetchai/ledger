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

#include "core/byte_array/consumers.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "ledger/chain/main_chain_remote_control.hpp"
#include "network/service/service_client.hpp"
#include "storage/document_store_protocol.hpp"
#include <iostream>

#include "core/byte_array/decoders.hpp"
#include "core/string/trim.hpp"
#include "ledger/chain/helper_functions.hpp"
using namespace fetch;

using namespace fetch::service;
using namespace fetch::byte_array;

int main(int argc, char const **argv)
{
  using service_type        = ServiceClient;
  using client_type         = fetch::network::TCPClient;
  using shared_service_type = std::shared_ptr<service_type>;

  fetch::logger.DisableLogger();
  commandline::ParamsParser params;
  params.Parse(argc, argv);

  std::cout << std::endl;
  fetch::commandline::DisplayCLIHeader("Main Chain Remote");

  // Remote setup
  fetch::network::NetworkManager tm(8);
  std::string                    host = "localhost";
  uint16_t                       port = params.GetParam<uint16_t>("port", 8080);

  chain::MainChainRemoteControl    remote;
  std::vector<shared_service_type> services;

  client_type client(tm);
  client.Connect(host, port);
  shared_service_type service = std::make_shared<service_type>(client, tm);
  services.push_back(service);
  remote.SetClient(service);

  tm.Start();

  // Setting tokenizer up
  enum
  {
    TOKEN_NAME      = 1,
    TOKEN_STRING    = 2,
    TOKEN_NUMBER    = 3,
    TOKEN_CATCH_ALL = 12
  };
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
          command.push_back(t);
      }

      if (command.size() > 0)
      {
        if (command[0] == "connect")
        {
          if (command.size() == 3)
          {
            remote.Connect(command[1], uint16_t(command[2].AsInt()));
          }
          else
          {
            std::cout << "usage: connect [mainchain] [ip] [port]" << std::endl;
          }
        }

        if (command[0] == "addblock")
        {
          if (command.size() == 1)
          {
            //            remote.AddBlock();
          }
          else
          {
            std::cout << "usage: addblock" << std::endl;
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
