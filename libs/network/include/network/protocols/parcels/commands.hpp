#ifndef PROTOCOLS_SWARM_PARCELSCOMMANDS_HPP
#define PROTOCOLS_SWARM_PARCELSCOMMANDS_HPP

namespace fetch
{
namespace protocols
{

struct SwarmParcels
{
enum
{
  CLIENT_NEEDS_PARCEL_IDS = 1,
  CLIENT_NEEDS_PARCEL_DATA = 2,
};
};
}
}

#endif
