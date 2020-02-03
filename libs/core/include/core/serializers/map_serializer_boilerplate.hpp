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

namespace fetch {
namespace serializers {

template <class T, class D, class... Fields>
struct MapSerializerBoilerplate
{
  using Type       = T;
  using DriverType = D;

  template <class Constructor>
  static constexpr void Serialize(Constructor &map_constructor, Type const &v)
  {
    constexpr auto map_size = value_util::Sum(Fields::LOGICAL_SIZE...);

    value_util::ForEach(
        [map = map_constructor(map_size), &v](auto field) { field.Serialize(map, v); },
        Fields{}...);
  }

  template <class MapDeserializer>
  static constexpr void Deserialize(MapDeserializer &map, Type &v)
  {
    value_util::ForEach([&map, &v](auto field) { field.Deserialize(map, v); }, Fields{}...);
  }
};

struct ValueSerializer
{
  static constexpr std::size_t LOGICAL_SIZE = 1;
};

struct ExtraChecks
{
  static constexpr std::size_t LOGICAL_SIZE = 0;
};

template <uint8_t KEY, class Underlying = void>
struct SimplySerializedAs : ValueSerializer
{
  template <class Map, class Object>
  static constexpr void Serialize(Map &map, Object const &object)
  {
    map.Append(KEY, static_cast<Underlying const &>(object));
  }

  template <class Map, class Object>
  static constexpr void Deserialize(Map &map, Object &object)
  {
    map.ExpectKeyGetValue(Key, static_cast<Underlying &>(object));
  }
};

template <uint8_t KEY>
struct SimplySerializedAs<KEY, void> : ValueSerializer
{
  template <class Map, class Object>
  static constexpr void Serialize(Map &map, Object const &object)
  {
    map.Append(KEY, object);
  }

  template <class Map, class Object>
  static constexpr void Deserialize(Map &map, Object &object)
  {
    map.ExpectKeyGetValue(Key, object);
  }
};

template <uint8_t KEY, class MemberVariable, MemberVariable MEMBER_VARIABLE,
          class Underlying = void>
struct ExpectedKeyMember : ValueSerializer
{
  template <class Map, class Object>
  static constexpr void Serialize(Map &map, Object const &object)
  {
    map.Append(KEY, static_cast<Underlying const &>(object.*MEMBER_VARIABLE));
  }

  template <class Map, class Object>
  static constexpr void Deserialize(Map &map, Object &object)
  {
    map.ExpectKeyGetValue(KEY, static_cast<Underlying &>(object.*MEMBER_VARIABLE));
  }
};

template <uint8_t KEY, class MemberVariable, MemberVariable MEMBER_VARIABLE>
struct ExpectedKeyMember<KEY, MemberVariable, MEMBER_VARIABLE, void> : ValueSerializer
{
  template <class Map, class Object>
  static constexpr void Serialize(Map &map, Object const &object)
  {
    map.Append(KEY, object.*MEMBER_VARIABLE);
  }

  template <class Map, class Object>
  static constexpr void Deserialize(Map &map, Object &object)
  {
    map.ExpectKeyGetValue(KEY, object.*MEMBER_VARIABLE);
  }
};

#define EXPECTED_KEY_MEMBER(KEY, ...) ExpectedKeyMember<KEY, decltype(&__VA_ARGS__), &__VA_ARGS__>
#define EXPECTED_KEY_MEMBER_AS(KEY, member, Type) \
  ExpectedKeyMember<KEY, decltype(&member), &member, Type>

}  // namespace serializers
}  // namespace fetch
