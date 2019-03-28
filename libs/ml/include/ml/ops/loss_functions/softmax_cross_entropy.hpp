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

#include "math/free_functions/ml/activation_functions/softmax.hpp"
#include "math/free_functions/ml/loss_functions/cross_entropy.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class SoftmaxCrossEntropy
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  SoftmaxCrossEntropy()          = default;
  virtual ~SoftmaxCrossEntropy() = default;

  virtual typename ArrayType::Type Forward(std::vector<ArrayType> const &inputs)
  {
    // third term may be present for specifying n_classes
    assert((inputs.size() == 2) || ((inputs.size() == 3) && (inputs.at(2).size() == 1)));
    assert(inputs.at(0).size() == inputs.at(1).size());

    // sanity check the softmax adds up to 1
    assert(Sum(fetch::math::Softmax(inputs[0])) - (DataType(1)) < 0.0001);

    SizeType n_classes{2};
    if (inputs.size() == 3)
    {
      n_classes = SizeType(inputs.at(2).At(0));
    }

    // softmax forward & then CrossEntropy
    typename ArrayType::Type result =
        fetch::math::CrossEntropyLoss(fetch::math::Softmax(inputs[0]), inputs[1], n_classes);

    return result;
  }

  virtual ArrayType Backward(std::vector<ArrayType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].size() == inputs[1].size());

    return fetch::math::Subtract(inputs[0], inputs[1]);
  }

  static constexpr char const *DESCRIPTOR = "SoftmaxCrossEntropy";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch