#ifndef PROTOCOLS_FETCH_PROTOCOLS_HPP
#define PROTOCOLS_FETCH_PROTOCOLS_HPP

namespace fetch
{
namespace protocols
{

struct FetchProtocols
{

enum
{
  SWARM        = 1,
  SHARD        = 2,
  AEA_TO_NODE  = 64,
  NODE_TO_AEA  = 65,
  NODE_TO_NODE = 66
};

};


};
};


#endif
