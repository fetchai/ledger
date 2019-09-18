#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include <cstdint>
#include <string>

namespace fetch {
namespace ledger {

class ServiceIdentifier
{
public:
  enum class Type : uint8_t
  {
    INVALID = 0,
    CORE    = 1,
    HTTP,
    DKG,
    LANE = 10,
  };

  // Construction / Destruction
  ServiceIdentifier() = default;
  explicit ServiceIdentifier(Type type, int32_t instance = -1);
  ServiceIdentifier(ServiceIdentifier const &) = default;
  ~ServiceIdentifier()                         = default;

  Type        type() const;
  int32_t     instance() const;
  std::string ToString() const;

  // Operators
  bool               operator==(ServiceIdentifier const &other) const;
  ServiceIdentifier &operator=(ServiceIdentifier const &) = default;

private:
  Type    type_{Type::INVALID};
  int32_t instance_{-1};

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

inline ServiceIdentifier::Type ServiceIdentifier::type() const
{
  return type_;
}

inline int32_t ServiceIdentifier::instance() const
{
  return instance_;
}

char const *ToString(ServiceIdentifier::Type type);

}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::ServiceIdentifier, D>
{
public:
  using DriverType = D;
  using Type       = ledger::ServiceIdentifier;

  static const uint8_t TYPE     = 1;
  static const uint8_t INSTANCE = 2;

  template <typename T>
  static inline void Serialize(T &map_constructor, Type const &x)
  {
    auto map = map_constructor(2);
    map.Append(TYPE, static_cast<uint8_t>(x.type_));
    map.Append(INSTANCE, x.instance_);
  }

  template <typename T>
  static inline void Deserialize(T &map, Type &x)
  {
    uint8_t raw_type{0};

    map.ExpectKeyGetValue(TYPE, raw_type);
    map.ExpectKeyGetValue(INSTANCE, x.instance_);

    x.type_ = static_cast<Type::Type>(raw_type);
  }
};

}  // namespace serializers
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::ledger::ServiceIdentifier>
{
  std::size_t operator()(fetch::ledger::ServiceIdentifier const &id) const
  {
    static_assert(sizeof(std::size_t) == 8, "Assumed size of size_t");

    std::size_t result{0};

    // manipulate the result into two 32fields one for the type and one for the instance number
    auto raw = reinterpret_cast<uint32_t *>(&result);
    raw[0]   = static_cast<uint32_t>(id.type());
    raw[1]   = static_cast<uint32_t>(id.instance());

    return result;
  }
};

}  // namespace std
