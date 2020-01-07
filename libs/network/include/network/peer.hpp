#pragma once
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

#include "core/serializers/group_definitions.hpp"
#include "logging/logging.hpp"

#include <cstdint>
#include <string>

namespace fetch {
namespace network {

class Peer
{
public:
  // Construction / Destruction
  Peer() = default;
  explicit Peer(char const *address)
    : Peer(std::string{address})
  {}
  explicit Peer(std::string const &address);
  explicit Peer(std::string address, uint16_t port)
    : address_{std::move(address)}
    , port_{port}
  {}
  Peer(Peer const &)     = default;
  Peer(Peer &&) noexcept = default;
  ~Peer()                = default;

  bool Parse(std::string const &address);

  void Update(std::string address, uint16_t port)
  {
    address_ = std::move(address);
    port_    = port;
  }

  std::string const &address() const
  {
    return address_;
  }

  uint16_t port() const
  {
    return port_;
  }

  std::string ToString() const;
  std::string ToUri() const;

  Peer &operator=(Peer const &) = default;
  Peer &operator=(Peer &&) = default;

  bool operator==(Peer const &other) const noexcept;
  bool operator<(Peer const &other) const;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

private:
  std::string address_{"localhost"};
  uint16_t    port_{0};
};

inline std::string Peer::ToString() const
{
  return address_ + ':' + std::to_string(port_);
}

inline std::string Peer::ToUri() const
{
  return "tcp://" + address_ + ':' + std::to_string(port_);
}

inline bool Peer::operator==(Peer const &other) const noexcept
{
  return ((address_ == other.address_) && (port_ == other.port_));
}

inline bool Peer::operator<(Peer const &other) const
{
  bool r = false;
  if (address_ < other.address_)
  {
    r = true;
  }
  else if (address_ > other.address_)
  {
    r = false;
  }
  else if (port_ < other.port_)
  {
    r = true;
  }
  return r;
}

inline std::ostream &operator<<(std::ostream &s, Peer const &peer)
{
  s << peer.ToString();
  return s;
}

}  // namespace network

namespace serializers {
template <typename D>
struct MapSerializer<network::Peer, D>
{
public:
  using Type       = network::Peer;
  using DriverType = D;

  static const uint8_t ADDRESS = 1;
  static const uint8_t PORT    = 2;

  template <typename T>
  static void Serialize(T &map_constructor, Type const &peer)
  {
    auto map = map_constructor(2);
    map.Append(ADDRESS, peer.address_);
    map.Append(PORT, peer.port_);
  }

  template <typename T>
  static void Deserialize(T &map, Type &peer)
  {
    map.ExpectKeyGetValue(ADDRESS, peer.address_);
    map.ExpectKeyGetValue(PORT, peer.port_);
  }
};
}  // namespace serializers

}  // namespace fetch

namespace std {

template <>
struct hash<fetch::network::Peer>
{
  std::size_t operator()(fetch::network::Peer const &peer) const noexcept
  {
    return std::hash<std::string>{}(peer.ToString());
  }
};

}  // namespace std
