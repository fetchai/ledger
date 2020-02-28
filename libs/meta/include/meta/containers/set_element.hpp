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

#include <utility>

namespace fetch {
namespace meta {

template <typename Container, typename Value>
bool IsIn(Container const &container, Value const &value)
{
  return container.find(value) != container.end();
}

namespace detail_ {

template <class Value, class Key>
Value ValueFrom(std::pair<Key, Value> const &element)
{
  return element.second;
}

template <class Value>
Value ValueFrom(Value const &element)
{
  return element;
}

class ValueType
{
  template <class Container>
  static constexpr typename Container::mapped_type MappedType(Container &&) noexcept;
  template <class Container, class... ImaginaryArgs>
  static constexpr typename Container::value_type MappedType(Container &&,
                                                             ImaginaryArgs &&...) noexcept;

public:
  template <class Container>
  using type = decltype(MappedType(std::declval<Container>()));
};

template <class Container>
using ValueTypeT = ValueType::type<Container>;

}  // namespace detail_

template <typename Container, typename Key, typename Value = detail_::ValueTypeT<Container>>
Value Lookup(Container const &container, Key const &key, Value default_value = {})
{
  auto it = container.find(key);
  if (it == container.end())
  {
    return std::move(default_value);
  }

  return detail_::ValueFrom<Value>(*it);
}

}  // namespace meta
}  // namespace fetch
