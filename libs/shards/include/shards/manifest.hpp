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

#include "core/byte_array/const_byte_array.hpp"
#include "shards/manifest_entry.hpp"

#include <cstddef>
#include <unordered_map>

namespace fetch {
namespace variant {
class Variant;
}
namespace shards {

class Manifest
{
public:
  using ServiceMap     = std::unordered_map<ServiceIdentifier, ManifestEntry>;
  using iterator       = ServiceMap::iterator;
  using const_iterator = ServiceMap::const_iterator;
  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  Manifest()                 = default;
  Manifest(Manifest const &) = default;
  Manifest(Manifest &&)      = default;
  ~Manifest()                = default;

  // Basic Iteration
  std::size_t    size() const;
  iterator       begin();
  iterator       end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  iterator       FindService(ServiceIdentifier const &service);
  const_iterator FindService(ServiceIdentifier const &service) const;
  const_iterator FindService(ServiceIdentifier::Type service_type) const;

  void AddService(ServiceIdentifier const &id, ManifestEntry const &entry);
  void AddService(ServiceIdentifier const &id, ManifestEntry &&entry);

  std::string FindExternalAddress(ServiceIdentifier::Type type,
                                  uint32_t index = ServiceIdentifier::SINGLETON_SERVICE) const;

  bool Parse(ConstByteArray const &text);

  // Operators
  Manifest &operator=(Manifest const &) = default;
  Manifest &operator=(Manifest &&) = default;

private:
  using Variant = variant::Variant;

  bool ExtractSection(Variant const &obj, ServiceIdentifier const &service);

  ServiceMap service_map_;

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

}  // namespace shards

namespace serializers {

template <typename D>
struct MapSerializer<shards::Manifest, D>
{
public:
  using DriverType = D;
  using Type       = shards::Manifest;

  static const uint8_t SERVICE_MAP = 1;

  template <typename T>
  static inline void Serialize(T &map_constructor, Type const &x)
  {
    auto map = map_constructor(1);
    map.Append(SERVICE_MAP, x.service_map_);
  }

  template <typename T>
  static inline void Deserialize(T &map, Type &x)
  {
    map.ExpectKeyGetValue(SERVICE_MAP, x.service_map_);
  }
};

}  // namespace serializers

}  // namespace fetch
