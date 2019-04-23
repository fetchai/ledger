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

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace math {

using SizeType   = uint64_t;
using SizeVector = std::vector<SizeType>;
using SizeSet    = std::unordered_set<SizeType>;

constexpr SizeType NO_AXIS = SizeType(-1);

}  // namespace math
}  // namespace fetch
