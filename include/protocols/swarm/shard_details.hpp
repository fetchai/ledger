#ifndef PROTOCOLS_SWARM_SHARD_DETAILS_HPP
#define PROTOCOLS_SWARM_SHARD_DETAILS_HPP
#include"protocols/swarm/entry_point.hpp"
namespace fetch
{
namespace protocols
{

struct ShardDetails 
{
  uint64_t handle;   // TODO: Make global

  EntryPoint entry_for_swarm;
  EntryPoint entry_for_peer;  
};


};
};

#endif
