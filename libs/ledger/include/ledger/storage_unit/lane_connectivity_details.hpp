#ifndef LEDGER_STORAGE_UNIT_LANE_CONNECTIVITY_DETAILS_HPP
#define LEDGER_STORAGE_UNIT_LANE_CONNECTIVITY_DETAILS_HPP

#include<atomic>

namespace fetch
{
namespace ledger
{

struct LaneConnectivityDetails
{
  LaneConnectivityDetails() 
    :
    transaction_pointer( 0 ),
    is_controller( false ),
    is_peer( false ),
    is_outgoing( false ) 
  {
    
  }

  std::atomic< int64_t > transaction_pointer;
  
  std::atomic<bool> is_controller;
  std::atomic<bool> is_peer;
  std::atomic<bool> is_outgoing;
};


}
}


#endif
