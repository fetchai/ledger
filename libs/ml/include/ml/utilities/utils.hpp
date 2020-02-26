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

#include "math/base_types.hpp"

#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace utilities {

std::string GetStrTimestamp();

template <typename T>
struct BM_Tensor_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_Tensor_config(T const &state)
  {
    auto size_len = static_cast<SizeType>(state.range(0));

    shape.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      shape.emplace_back(static_cast<SizeType>(state.range(1 + i)));
    }
  }

  std::vector<SizeType> shape;  // layers input/output sizes
};

/**
 * This converts a vector of shared_ptr<TensorType> to a vector of Tensor shapes.
 * @tparam TensorType
 * @param inputs vector of Tensor ptrs
 * @return vector of shapes of the tensors
 */
template <class TensorType>
std::vector<math::SizeVector> TensorPtrsToSizes(
    std::vector<std::shared_ptr<TensorType>> const &inputs)
{
  std::vector<math::SizeVector> input_shapes{};
  input_shapes.reserve(inputs.size());
  for (auto const &inp : inputs)
  {
    input_shapes.emplace_back(inp->shape());
  }
  return input_shapes;
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
