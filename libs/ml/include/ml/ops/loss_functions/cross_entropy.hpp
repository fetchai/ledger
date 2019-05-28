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

#include "math/fundamental_operators.hpp"
#include "math/ml/activation_functions/sigmoid.hpp"
#include "math/ml/activation_functions/softmax.hpp"
#include "math/ml/loss_functions/cross_entropy.hpp"
#include "ml/ops/loss_functions/criterion.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class CrossEntropy : public Criterion<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  CrossEntropy()          = default;
  virtual ~CrossEntropy() = default;

  /**
   * @param inputs vector of 2 one-hot Tensors
   * @return CrossEntropy value of 2 inputs
   */
  virtual DataType Forward(std::vector<ArrayType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).size() == inputs.at(1).size());

    SizeType n_classes{inputs.at(0).size()};
    DataType result = fetch::math::CrossEntropyLoss(inputs[0], inputs[1], n_classes);

    return result;
  }

  /**
   * @param inputs vector of 2 one-hot Tensors
   * @return gradient of CrossEntropy of 2 inputs
   */
  virtual ArrayType Backward(std::vector<ArrayType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].size() == inputs[1].size());
    assert(inputs[0].shape().size() == 2);

    ArrayType ret = fetch::math::Softmax(inputs[0], 0);
    fetch::math::Divide(inputs[1], ret, ret);
    fetch::math::Multiply(DataType(-1), ret, ret);

    return ret;
  }

  static constexpr char const *DESCRIPTOR = "CrossEntropy";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
