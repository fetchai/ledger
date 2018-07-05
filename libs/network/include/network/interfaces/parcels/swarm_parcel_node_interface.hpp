#ifndef SWARM_PARCEL_NODE_INTERFACE__
#define SWARM_PARCEL_NODE_INTERFACE__

#include <iostream>
#include <string>

#include "network/protocols/fetch_protocols.hpp"

namespace fetch
{
namespace swarm
{

class SwarmParcelProtocol;

class SwarmParcelNodeInterface
{
public:
  enum {
    protocol_number = fetch::protocols::FetchProtocols::PARCEL
  };
  typedef SwarmParcelProtocol protocol_class_type;

  SwarmParcelNodeInterface()
  {
  }

  virtual ~SwarmParcelNodeInterface()
  {
  }

  virtual std::string ClientNeedParcelList(const std::string &type, unsigned int count) = 0;
  virtual std::string ClientNeedParcelData(const std::string &type, const std::string &parcelname) = 0;
};

}
}

#endif
