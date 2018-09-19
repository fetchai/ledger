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
static const std::regex IDENTITY_FORMAT("^[a-zA-Z0-9/+]{86}==$");

static const std::regex PEER_FORMAT("^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+:[0-9]+$");
static const std::regex TCP_FORMAT("^tcp://([^:]+):([0-9]+)$");

Uri::Uri(ConstByteArray const &uri)
{
  if (!Parse(uri))
  {
    throw std::runtime_error("Unable to parse URI");
  }
}

bool Uri::Parse(ConstByteArray const &uri)
{
  bool success = false;

  std::string const data = static_cast<std::string>(uri);

  std::smatch matches;
  std::regex_match(data, matches, URI_FORMAT);
  if (matches.size() == 3)
  {
    std::string const &scheme = matches[1];
    std::string const &authority = matches[2];

    if (scheme == "tcp")
    {
      success    = tcp_.Parse(authority);
      scheme_    = Scheme::Tcp;
      authority_ = authority;
    }
    else if (scheme == "muddle")
    {
      success    = std::regex_match(authority, IDENTITY_FORMAT);
      scheme_    = Scheme::Muddle;
      authority_ = authority;
    }
  }

  // only update the URI if value
  if (success)
  {
    uri_ = uri;
  }

  return success;
}

} // namespace p2p
} // namespace fetch
