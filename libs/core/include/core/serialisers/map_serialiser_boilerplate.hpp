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

#include "meta/value_util.hpp"

#include <type_traits>

namespace fetch {
namespace serialisers {

template <class T, class D, class... Fields>
struct MapSerialiserBoilerplate
{
  using Type       = T;
  using DriverType = D;

  template <class Constructor>
  static constexpr void Serialise(Constructor &map_constructor, Type const &v)
  {
    constexpr auto map_size = value_util::Sum(Fields::LogicalSize()...);

    value_util::ForEach(
        [map = map_constructor(map_size), &v](auto field) mutable { field.Serialise(map, v); },
        Fields{}...);
  }

  template <class MapDeserialiser>
  static constexpr void Deserialise(MapDeserialiser &map, Type &v)
  {
    value_util::ForEach([&map, &v](auto field) { field.Deserialise(map, v); }, Fields{}...);
  }
};

namespace detail_ {

template <class Underlying, class Map, class Key, class Actual>
std::enable_if_t<std::is_base_of<Underlying, Actual>::value> SerialiseAs(Map &map, Key key,
                                                                         Actual const &actual)
{
  map.Append(key, static_cast<Underlying const &>(actual));
}

template <class Underlying, class Map, class Key, class Actual>
std::enable_if_t<!std::is_base_of<Underlying, Actual>::value> SerialiseAs(Map &map, Key key,
                                                                          Actual actual)
{
  map.Append(key, static_cast<Underlying>(actual));
}

template <class Underlying, class Map, class Key, class Actual>
std::enable_if_t<std::is_base_of<Underlying, Actual>::value> DeserialiseAs(Map &map, Key key,
                                                                           Actual &actual)
{
  map.ExpectKeyGetValue(key, static_cast<Underlying &>(actual));
}

template <class Underlying, class Map, class Key, class Actual>
std::enable_if_t<!std::is_base_of<Underlying, Actual>::value> DeserialiseAs(Map &map, Key key,
                                                                            Actual &actual)
{
  Underlying raw_data;
  map.ExpectKeyGetValue(key, raw_data);
  actual = static_cast<Actual>(raw_data);
}

}  // namespace detail_

struct ValueSerialiser
{
  static constexpr std::size_t LogicalSize() noexcept
  {
    return 1;
  }
};

struct ExtraChecks
{
  static constexpr std::size_t LogicalSize() noexcept
  {
    return 0;
  }
};

template <uint8_t KEY, class Underlying = void>
struct SimplySerialisedAs : ValueSerialiser
{
  template <class Map, class Object>
  static constexpr void Serialise(Map &map, Object const &object)
  {
    detail_::SerialiseAs<Underlying>(map, KEY, object);
  }

  template <class Map, class Object>
  static constexpr void Deserialise(Map &map, Object &object)
  {
    detail_::DeserialiseAs<Underlying>(map, KEY, object);
  }
};

template <uint8_t KEY>
struct SimplySerialisedAs<KEY, void> : ValueSerialiser
{
  template <class Map, class Object>
  static constexpr void Serialise(Map &map, Object const &object)
  {
    map.Append(KEY, object);
  }

  template <class Map, class Object>
  static constexpr void Deserialise(Map &map, Object &object)
  {
    map.ExpectKeyGetValue(KEY, object);
  }
};

template <uint8_t KEY, class MemberVariable, MemberVariable MEMBER_VARIABLE,
          class Underlying = void>
struct SerialisedStructField : ValueSerialiser
{
  template <class Map, class Object>
  static constexpr void Serialise(Map &map, Object const &object)
  {
    detail_::SerialiseAs<Underlying>(map, KEY, object.*MEMBER_VARIABLE);
  }

  template <class Map, class Object>
  static constexpr void Deserialise(Map &map, Object &object)
  {
    detail_::DeserialiseAs<Underlying>(map, KEY, object.*MEMBER_VARIABLE);
  }
};

template <uint8_t KEY, class MemberVariable, MemberVariable MEMBER_VARIABLE>
struct SerialisedStructField<KEY, MemberVariable, MEMBER_VARIABLE, void> : ValueSerialiser
{
  template <class Map, class Object>
  static constexpr void Serialise(Map &map, Object const &object)
  {
    map.Append(KEY, object.*MEMBER_VARIABLE);
  }

  template <class Map, class Object>
  static constexpr void Deserialise(Map &map, Object &object)
  {
    map.ExpectKeyGetValue(KEY, object.*MEMBER_VARIABLE);
  }
};

template <uint8_t KEY, class FormalType>
struct Deprecated : ValueSerialiser
{
  template <class Map, class Object>
  static constexpr void Serialiser(Map &map, Object const & /*unused*/)
  {
    map.Append(KEY, FormalType());
  }
  template <class Map, class Object>
  static constexpr void Deserialise(Map &map, Object & /*unused*/)
  {
    FormalType discarded;
    map.ExpectKeyGetValue(KEY, discarded);
  }
};

#define serialiseD_STRUCT_FIELD(KEY, ...) \
  serialisers::SerialisedStructField<KEY, decltype(&__VA_ARGS__), &__VA_ARGS__>
#define serialiseD_STRUCT_FIELD_AS(KEY, member, Type) \
  serialisers::SerialisedStructField<KEY, decltype(&member), &member, Type>

}  // namespace serialisers
}  // namespace fetch
