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

#include "ml/ops/loss_functions.hpp"
#include "ml/ops/utils.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MeanSquareErrorLayer
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  MeanSquareErrorLayer()          = default;
  virtual ~MeanSquareErrorLayer() = default;

  virtual float Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->shape() == inputs[1]->shape());
    ArrayType result = fetch::math::MeanSquareError(*inputs[0], *inputs[1]);
    return result(0, 0);   
  }

  virtual ArrayPtrType Backward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->shape() == inputs[1]->shape());
    ArrayPtrType ret = std::make_shared<ArrayType>(inputs[0]->shape());
    fetch::math::Subtract(*inputs[0], *inputs[1], *ret);
    return ret;
  }

};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
