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

#include "core/serialisers/group_definitions.hpp"
#include "core/serialisers/map_serialiser_boilerplate.hpp"
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

  template <typename D>
  struct MapSerialiser;

private:
  muddle::Address address_{};
  network::Uri    uri_{};
  uint16_t        local_port_{0};

  template <uint8_t KEY, class MemberVariable, MemberVariable MEMBER_VARIABLE, class Underlying>
  friend struct SerialisedStructField;
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

template <typename D>
struct ManifestEntry::MapSerialiser
  : serialisers::MapSerialiserBoilerplate<ManifestEntry, D,
                                          serialiseD_STRUCT_FIELD(1, ManifestEntry::uri_),
                                          serialiseD_STRUCT_FIELD(2, ManifestEntry::local_port_),
                                          serialiseD_STRUCT_FIELD(3, ManifestEntry::address_)>
{
};

}  // namespace shards

namespace serialisers {

template <typename D>
struct MapSerialiser<shards::ManifestEntry, D> : shards::ManifestEntry::MapSerialiser<D>
{
};

}  // namespace serialisers
}  // namespace fetch
