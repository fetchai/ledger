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

#include<sstream>
using namespace fetch;
using namespace fetch::ledger;

int main(int argc, char const **argv) 
{
  std::cout << "Was here?" << std::endl;
  
//  fetch::logger.DisableLogger();
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint32_t lane_count =  params.GetParam<uint32_t>("lane-count", 1);  
     
  std::string dummy;
  fetch::commandline::DisplayCLIHeader("Multi-lane server");
  
  std::cout << "Starting " << lane_count << " lanes." << std::endl << std::endl;
  

  fetch::network::ThreadManager tm(8);
  std::vector< std::shared_ptr< LaneService > > lanes;
  for(uint32_t i = 0 ; i < lane_count ; ++i ) {
    lanes.push_back(std::make_shared< LaneService > (uint32_t(i), lane_count, uint16_t(8080 + i), tm ) );
  }
  
  tm.Start();


  
  std::cout << "Press ENTER to quit" << std::endl;                                       
  std::cin >> dummy;
  
  tm.Stop();


  return 0;
}
