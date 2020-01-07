//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "network/peer.hpp"

#include <regex>
#include <stdexcept>

static const std::regex ADDRESS_FORMAT("^(.*):(\\d+)$");
static const std::regex URI_ADDRESS_FORMAT("^tcp://(.*):(\\d+)$");

namespace fetch {
namespace network {

Peer::Peer(std::string const &address)
{
  if (!Parse(address))
  {
    throw std::runtime_error(std::string("Unable to parse input address:'") + address + "'");
  }
}

bool Peer::Parse(std::string const &address)
{
  std::smatch matches;

  if (address.empty())
  {
    Update("", 0);
    return true;
  }

  std::regex_match(address, matches, URI_ADDRESS_FORMAT);
  if (matches.size() == 3)
  {
    Update(matches[1], static_cast<uint16_t>(stol(matches[2])));
    return true;
  }

  std::regex_match(address, matches, ADDRESS_FORMAT);
  if (matches.size() == 3)
  {
    Update(matches[1], static_cast<uint16_t>(stol(matches[2])));
    return true;
  }

  return false;
}

}  // namespace network
}  // namespace fetch
