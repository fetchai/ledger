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
#include "vectorise/fixed_point/fixed_point.hpp"

#include <cstdint>

namespace fetch {
namespace serializers {

template <std::uint16_t I, std::uint16_t F, typename D>
struct ForwardSerializer<fixed_point::FixedPoint<I, F>, D>
{
  using Type       = fixed_point::FixedPoint<I, F>;
  using DriverType = D;

  template <typename Interface>
  static void Serialize(Interface &interface, Type const &n)
  {
    interface << n.Data();
  }

  template <typename Interface>
  static void Deserialize(Interface &interface, Type &n)
  {
    typename fixed_point::FixedPoint<I, F>::Type data;
    interface >> data;
    n = fixed_point::FixedPoint<I, F>::FromBase(data);
  }
};

}  // namespace serializers
}  // namespace fetch
