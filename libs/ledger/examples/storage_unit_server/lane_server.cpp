#include<iostream>
#include"storage/revertible_document_store.hpp"
#include"storage/document_store_protocol.hpp"
#include"storage/object_store.hpp"
#include"storage/object_store_protocol.hpp"
#include"storage/object_store_syncronisation_protocol.hpp"
#include"network/service/server.hpp"
#include"core/commandline/cli_header.hpp"
#include"core/commandline/parameter_parser.hpp"
#include"ledger/chain/transaction.hpp"
#include"ledger/storage_unit/lane_service.hpp"
#include"ledger/storage_unit/storage_unit_bundled_service.hpp"

#include<sstream>
using namespace fetch;
using namespace fetch::ledger;
int main(int argc, char const **argv) 
{  
  // Reading config
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint32_t lane_count =  params.GetParam<uint32_t>("lane-count", 8);
  uint16_t port =  params.GetParam<uint16_t>("port", 8080);
  std::string dbdir =  params.GetParam<std::string>("db-dir", "db1/");      
  int log = params.GetParam<int>("showlog", 0);
  if(log == 0) {
    fetch::logger.DisableLogger();
  }
    
  fetch::commandline::DisplayCLIHeader("Storage Unit Bundled Service");
  std::cout << "Starting " << lane_count << " lanes." << std::endl << std::endl;

  // Setting up
  fetch::network::NetworkManager tm(8);
  tm.Start();
  StorageUnitBundledService service;
  service.Setup(dbdir, lane_count, port, tm);    


  // Running until enter
  std::cout << "Press ENTER to quit" << std::endl;                                       
std::string dummy;
  std::cin >> dummy;
  
  tm.Stop();


  return 0;
}
