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

#include <memory>
#include <set>
#include <vector>

namespace fetch {
namespace semanticsearch {

using DBIndexType            = uint64_t;  ///< Database index type. Essentially pointer to record.
using SemanticCoordinateType = uint64_t;  ///< Base coordinate type semantic position.
                                          ///  Always internal 0 -> 1

using SemanticPosition = std::vector<SemanticCoordinateType>;  ///< Position in semantic space.

using DBIndexSet    = std::set<DBIndexType>;  ///< Set of indices used to return search results.
using DBIndexSetPtr = std::shared_ptr<DBIndexSet>;

}  // namespace semanticsearch
}  // namespace fetch
