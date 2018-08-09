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

  MainChainService service(dbdir, port, tm, "foo");

  // Running until enter
  std::cout << "Press ENTER to quit" << std::endl;
  std::string dummy;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
