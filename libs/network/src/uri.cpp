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

#include "network/uri.hpp"

#include <istream>
#include <ostream>
#include <regex>
#include <stdexcept>

namespace fetch {
namespace network {

static const std::regex URI_FORMAT("^([a-z]+)://(.*)$");
static const std::regex IDENTITY_FORMAT("^[a-zA-Z0-9/+]{86}==$");

static const std::regex PEER_FORMAT(R"(^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+:[0-9]+$)");
static const std::regex TCP_FORMAT("^tcp://([^:]+):([0-9]+)$");

Uri::Uri(Peer const &peer)
  : uri_(peer.ToUri())
  , scheme_(Scheme::Tcp)
  , authority_(peer.ToString())
  , tcp_(peer)
{}

Uri::Uri(ConstByteArray const &uri)
{
  if (!Parse(uri))
  {
    throw std::runtime_error("Unable to parse URI: " + std::string(uri));
  }
}

bool Uri::Parse(char const *uri)
{
  return Parse(std::string{uri});
}

bool Uri::Parse(ConstByteArray const &uri)
{
  return Parse(static_cast<std::string>(uri));
}

bool Uri::Parse(std::string const &uri)
{
  bool success = false;

  if (uri.empty())
  {
    return false;
  }

  std::smatch matches;
  std::regex_match(uri, matches, URI_FORMAT);
  if (matches.size() == 3)
  {
    std::string const &scheme    = matches[1];
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

bool Uri::IsTcpPeer() const
{
  return scheme_ == Scheme::Tcp;
}

bool Uri::IsMuddleAddress() const
{
  return scheme_ == Scheme::Muddle;
}

Peer const &Uri::GetTcpPeer() const
{
  assert(scheme_ == Scheme::Tcp);
  return tcp_;
}

byte_array::ConstByteArray const &Uri::GetMuddleAddress() const
{
  assert(scheme_ == Scheme::Muddle);
  return authority_;
}

std::string Uri::ToString() const
{
  switch (scheme_)
  {
  case Scheme::Tcp:
    return std::string("tcp://") + tcp_.ToString();
  case Scheme::Muddle:
    return std::string("muddle://") + std::string(authority_);
  case Scheme::Unknown:
  default:
    return "unknown:";
  }
}

std::ostream &operator<<(std::ostream &stream, Uri const &uri)
{
  stream << uri.ToString();
  return stream;
}

std::istream &operator>>(std::istream &stream, Uri &uri)
{
  std::string value{};
  stream >> value;

  if (!stream.fail())
  {
    if (!uri.Parse(value))
    {
      throw std::runtime_error("Failed to parse input peer: " + value);
    }
  }

  return stream;
}

}  // namespace network
}  // namespace fetch
