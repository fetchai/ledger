#include "network/peer.hpp"

#include <regex>
#include <stdexcept>

#include <iostream>

static const std::regex ADDRESS_FORMAT("^(.*):(\\d+)$");

namespace fetch {
namespace network {

Peer::Peer(std::string const &address)
{
  if (!Parse(address))
  {
    throw std::runtime_error("Unable to parse input address");
  }
}

bool Peer::Parse(std::string const &address)
{
  bool success = false;

  std::smatch matches;
  std::regex_match(address, matches, ADDRESS_FORMAT);

  if (matches.size() == 3)
  {
    Update(matches[1], static_cast<uint16_t>(stol(matches[2])));

    success = true;
  }

  return success;
}

}  // namespace network
}  // namespace fetch
