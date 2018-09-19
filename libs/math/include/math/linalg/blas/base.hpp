#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "vectorise/platform.hpp"
#include "vectorise/memory/shared_array.hpp"

namespace fetch {
namespace math {

template <typename SHARED_TYPE, std::size_t type_size>
class SharedArray;

template <typename RECTANGULAR_TYPE, typename RECTANGULAR_CONTAINER, bool PAD_HEIGHT, bool PAD_WIDTH>
class RectangularArray;

template <typename T, typename C, typename SUPER_TYPE>
class Matrix;

namespace linalg {

template <typename T, typename MATRIX, uint64_t S, uint64_t I,
          uint64_t V = platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
class Blas
{
public:

  template <typename... Args>
  void operator()(Args... args) = delete;
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch
