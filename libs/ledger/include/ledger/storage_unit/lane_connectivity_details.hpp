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
    is_controller( false ),
    is_peer( false ),
    is_outgoing( false ) 
  {
    
  }

  
  std::atomic<bool> is_controller;
  std::atomic<bool> is_peer;
  std::atomic<bool> is_outgoing;
};


}
}


#endif
