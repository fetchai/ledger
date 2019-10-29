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

#include <vector>

namespace fetch {
namespace core {

/**
 * vector that range checks access in debug mode
 * @tparam T
 */
template <typename T>
struct Vector : public std::vector<T>
{
  using std::vector<T>::vector;

  T &operator[](std::size_t index)
  {
#ifndef NDEBUG
    return this->at(index);
#else
    return std::vector<T>::operator[](index);
#endif
  }

  T const &operator[](std::size_t index) const
  {
#ifndef NDEBUG
    return this->at(index);
#else
    return std::vector<T>::operator[](index);
#endif
  }
};

}  // namespace core
}  // namespace fetch
