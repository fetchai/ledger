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
#include "math/free_functions/ml/loss_functions/cross_entropy.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class CrossEntropy
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  CrossEntropy()          = default;
  virtual ~CrossEntropy() = default;

  virtual typename ArrayType::Type Forward(std::vector<ArrayType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).size() == inputs.at(1).size());

    typename ArrayType::Type result = fetch::math::CrossEntropyLoss(inputs[0], inputs[1]);

    return result;
  }

  virtual ArrayType Backward(std::vector<ArrayType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].size() == inputs[1].size());

    typename ArrayType::Type n_classes = static_cast<typename ArrayType::Type>(inputs[1].size());
    ArrayType ret(fetch::math::Divide(fetch::math::Subtract(inputs[0], inputs[1]), n_classes));
    return ret;
  }

  static constexpr char const *DESCRIPTOR = "CrossEntropy";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
