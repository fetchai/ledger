#pragma once

#include "network/p2pservice/p2p_peer_details.hpp"

namespace fetch {
namespace p2p {
namespace details {

struct NodeDetailsImplementation
{
  mutable mutex::Mutex mutex;
  PeerDetails          details;
};

}  // namespace details

using NodeDetails = std::shared_ptr<details::NodeDetailsImplementation>;

inline NodeDetails MakeNodeDetails()
{
  return std::make_shared<details::NodeDetailsImplementation>();
}

}  // namespace p2p
}  // namespace fetch
