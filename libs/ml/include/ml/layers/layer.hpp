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

#include "ml/ops/placeholder.hpp"
#include "ml/ops/weights.hpp"
#include "ml/subgraph.hpp"

#include <cmath>
#include <random>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class Layer : public SubGraph<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  std::uint64_t in_size;
  std::uint64_t out_size;

  Layer(std::uint64_t in, std::uint64_t out)
    : in_size(in)
    , out_size(out)
  {}

  /**
   * xavier weights initialisation
   * @param weights
   */
  void InitialiseWeights(ArrayPtrType weights)
  {
    std::random_device rd{};
    std::mt19937       gen{rd()};

    // http://proceedings.mlr.press/v9/glorot10a/glorot10a.pdf
    std::normal_distribution<> rng(0, std::sqrt(2.0 / double(in_size + out_size)));
    for (std::uint64_t i(0); i < weights->size(); ++i)
    {
      weights->At(i) = typename ArrayType::Type(rng(gen));
    }
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
