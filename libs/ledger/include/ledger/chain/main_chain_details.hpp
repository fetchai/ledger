#pragma once

#include<atomic>

namespace fetch
{
namespace chain
{

struct MainChainDetails
{
  MainChainDetails() 
    :
    is_controller( false ),
    is_peer( false ),
    is_miner( false),
    is_outgoing( false ) 
  {
    
  }

  
  std::atomic<bool> is_controller;
  std::atomic<bool> is_peer;
  std::atomic<bool> is_miner;  
  std::atomic<bool> is_outgoing;
};


}
}


