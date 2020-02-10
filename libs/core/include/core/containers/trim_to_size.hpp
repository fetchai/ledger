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

#include <cstddef>
#include <cstdint>
#include <iterator>

namespace fetch {
namespace core {

template <typename T>
void TrimToSize(T &container, uint64_t max_allowed)
{
  if (container.size() > max_allowed)
  {
    auto const num_to_remove = container.size() - max_allowed;

    auto end = container.begin();
    std::advance(end, static_cast<std::ptrdiff_t>(num_to_remove));

    container.erase(container.begin(), end);
  }
}

}  // namespace core
}  // namespace fetch
