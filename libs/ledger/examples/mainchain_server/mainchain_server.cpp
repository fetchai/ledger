//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "ledger/chain/main_chain_service.hpp"
#include "ledger/chain/transaction.hpp"
#include "network/service/server.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/object_store.hpp"
#include "storage/object_store_protocol.hpp"
#include "storage/object_store_syncronisation_protocol.hpp"
#include "storage/revertible_document_store.hpp"
#include <iostream>

#include <sstream>
using namespace fetch;
using namespace fetch::chain;
int main(int argc, char const **argv)
{
  // Reading config
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint16_t    port  = params.GetParam<uint16_t>("port", 8080);
  std::string dbdir = params.GetParam<std::string>("db-dir", "db1/");
  int         log   = params.GetParam<int>("showlog", 0);
  if (log == 0)
  {
    fetch::logger.DisableLogger();
  }

  fetch::commandline::DisplayCLIHeader("Main Chain Service");

  // Setting up
  fetch::network::NetworkManager tm(8);
  tm.Start();

  MainChainService service(dbdir, port, tm);

  // Running until enter
  std::cout << "Press ENTER to quit" << std::endl;
  std::string dummy;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
