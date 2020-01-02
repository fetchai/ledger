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
#include "muddle/address.hpp"
#include "network/uri.hpp"
#include "shards/service_identifier.hpp"

namespace fetch {
namespace shards {

class ManifestEntry
{
public:
  // Construction / Destruction
  ManifestEntry() = default;
  explicit ManifestEntry(network::Peer const &peer);
  explicit ManifestEntry(network::Uri const &uri);
  ManifestEntry(network::Uri uri, uint16_t local_port);
  ManifestEntry(ManifestEntry const &) = default;
  ManifestEntry(ManifestEntry &&)      = default;
  ~ManifestEntry()                     = default;

  muddle::Address const &address() const;
  network::Uri const &   uri() const;
  uint16_t               local_port() const;

  void UpdateAddress(muddle::Address address);

  // Operators
  ManifestEntry &operator=(ManifestEntry const &) = default;
  ManifestEntry &operator=(ManifestEntry &&) = default;

private:
  muddle::Address address_{};
  network::Uri    uri_{};
  uint16_t        local_port_{0};

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

inline muddle::Address const &ManifestEntry::address() const
{
  return address_;
}

inline network::Uri const &ManifestEntry::uri() const
{
  return uri_;
}

inline uint16_t ManifestEntry::local_port() const
{
  return local_port_;
}

}  // namespace shards

namespace serializers {

template <typename D>
struct MapSerializer<shards::ManifestEntry, D>
{
public:
  using DriverType = D;
  using Type       = shards::ManifestEntry;

  static const uint8_t URI        = 1;
  static const uint8_t LOCAL_PORT = 2;
  static const uint8_t ADDRESS    = 3;

  template <typename T>
  static inline void Serialize(T &map_constructor, Type const &x)
  {
    auto map = map_constructor(3);
    map.Append(URI, x.uri_);
    map.Append(LOCAL_PORT, x.local_port_);
    map.Append(ADDRESS, x.address_);
  }

  template <typename T>
  static inline void Deserialize(T &map, Type &x)
  {
    map.ExpectKeyGetValue(URI, x.uri_);
    map.ExpectKeyGetValue(LOCAL_PORT, x.local_port_);
    map.ExpectKeyGetValue(ADDRESS, x.address_);
  }
};

}  // namespace serializers
}  // namespace fetch
