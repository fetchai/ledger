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

  static const std::regex URI_FORMAT("^([a-z]+)://(.*)$");
  static const std::regex PEER_FORMAT("^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+:[0-9]+$");

  static const std::regex TCP_FORMAT("^tcp://([^:]+):([0-9]+)$");

  Uri::Uri(std::string const &uristr)
  {
    data_ = uristr;
    if (data_.length() == 0)
    {
      return;
    }
    {
      std::smatch matches;
      std::regex_match(data_, matches, URI_FORMAT);
      if (matches.size() == 3)
      {
        return;
      }
    }
    {
      std::smatch matches;
      std::regex_match(data_, matches, PEER_FORMAT);
      if (matches.size() == 1)
      {
        data_ = "tcp://" + uristr;
        return;
      }
    }
    data_ = "";
    FETCH_LOG_ERROR(LOGGING_NAME, "'", uristr, "' is not a valid uri during initialisation.");
  }

std::string Uri::GetProtocol() const
{
  std::smatch matches;
  std::regex_match(data_, matches, URI_FORMAT);
  if (matches.size() == 3)
  {
    return std::string(matches[1]);
  }
  throw std::invalid_argument(std::string("'") + data_ + "' is not a valid uri getting protocol.");
}

std::string Uri::GetRemainder() const
{
  std::smatch matches;
  std::regex_match(data_, matches, URI_FORMAT);
  if (matches.size() == 3)
  {
    return std::string(matches[2]);
  }
  throw std::invalid_argument(std::string("'") + data_ + "' is not a valid uri getting remainder.");
}

bool Uri::ParseTCP(byte_array::ByteArray &b, uint16_t &port) const
{
  std::smatch matches;
  std::regex_match(data_, matches, TCP_FORMAT);
  if (matches.size() == 3)
  {
    b = std::string(matches[1]);
    port = uint16_t(std::stoi(std::string(matches[2])));
    return true;
  }
  return false;
}


}
}
