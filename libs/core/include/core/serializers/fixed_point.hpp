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

#include "core/fixed_point/fixed_point.hpp"

namespace fetch {
namespace serializers {

template <typename T, std::uint16_t I, std::uint16_t F>
inline void Serialize(T &serializer, fixed_point::FixedPoint<I, F> const &n)
{
  serializer << n.Data();
}

template <typename T, std::uint16_t I, std::uint16_t F>
inline void Deserialize(T &serializer, fixed_point::FixedPoint<I, F> &n)
{
  typename fixed_point::FixedPoint<I, F>::Type data;
  serializer >> data;
  n = fixed_point::FixedPoint<I, F>::FromBase(data);
}

}  // namespace serializers
}  // namespace fetch
