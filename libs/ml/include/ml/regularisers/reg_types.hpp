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

#include "core/serialisers/base_types.hpp"
#include "core/serialisers/map_serialiser_boilerplate.hpp"

namespace fetch {
namespace ml {

enum class RegularisationType : uint8_t
{
  NONE,
  L1,
  L2,
};
}

namespace serialisers {

/**
 * serialiser for OpType
 * @tparam TensorType
 */
template <typename D>
struct MapSerialiser<ml::RegularisationType, D>
  : MapSerialiserBoilerplate<ml::RegularisationType, D, SimplySerialisedAs<1, uint8_t>>
{
};

}  // namespace serialisers
}  // namespace fetch
