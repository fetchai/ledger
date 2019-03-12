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

#include <cassert>
#include <memory>
#include <vector>

#include "math/free_functions/fundamental_operators.hpp"
#include "math/free_functions/ml/loss_functions/scaled_cross_entropy.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class ScaledCrossEntropy
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  ScaledCrossEntropy()          = default;
  virtual ~ScaledCrossEntropy() = default;

  virtual typename ArrayType::Type Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 3);
    assert(inputs[0]->size() == inputs[1]->size());
    assert(inputs[2]->size() == 1);

    typename ArrayType::Type result =
        fetch::math::ScaledCrossEntropyLoss(*inputs[0], *inputs[1], (*inputs[2])[0]);

    return result;
  }

  virtual ArrayPtrType Backward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->size() == inputs[1]->size());

    auto         n_classes = static_cast<typename ArrayType::Type>(inputs[1]->size());
    ArrayPtrType ret       = std::make_shared<ArrayType>(
        fetch::math::Divide(fetch::math::Subtract(*inputs[0], *inputs[1]), n_classes));

    for (std::size_t j = 0; j < ret->size(); ++j)
    {
      std::cout << "ret->At(j): " << ret->At(j) << std::endl;
    }

    return ret;
  }

  static constexpr char const *DESCRIPTOR = "ScaledCrossEntropy";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
