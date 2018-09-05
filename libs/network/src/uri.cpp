//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "network/uri.hpp"

#include <regex>
#include <stdexcept>


namespace fetch {
namespace network {

  static const std::regex URI_FORMAT("^([^:]*)://(.*)$");

std::string Uri::GetProtocol() const
{
  std::smatch matches;
  std::regex_match(data_, matches, URI_FORMAT);
  if (matches.size() == 3)
  {
    return std::string(matches[1]);
  }
  throw std::invalid_argument(std::string("'") + data_ + "' is not a valid uri.");
}

std::string Uri::GetRemainder() const
{
  std::smatch matches;
  std::regex_match(data_, matches, URI_FORMAT);
  if (matches.size() == 3)
  {
    return std::string(matches[2]);
  }
  throw std::invalid_argument(std::string("'") + data_ + "' is not a valid uri.");
}

}
}
