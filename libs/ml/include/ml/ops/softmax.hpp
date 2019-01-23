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

#include <assert.h>  // for assert

#include "math/free_functions/deep_learning/activation_functions.hpp"
#include "ml/ops/derivatives/derivatives.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class SoftmaxLayer : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  SoftmaxLayer()          = default;
  virtual ~SoftmaxLayer() = default;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 1);
    if (!this->output_ || this->output_->shape() != inputs[0]->shape())
    {
      this->output_ = std::make_shared<ArrayType>(inputs[0]->shape());
    }

    /*
     * Really naive implementation that relies only on ArrayType providing a At(size_t) method
     * TODO(private, 520) -- Clean up once we get unified ArrayType + operations
     */
    typename ArrayType::Type maxValue = std::numeric_limits<typename ArrayType::Type>::min();
    typename ArrayType::Type sum(0);
    for (size_t i(0); i < inputs[0]->size(); ++i)
    {
      maxValue = std::max(maxValue, inputs[0]->At(i));
    }
    for (size_t i(0); i < inputs[0]->size(); ++i)
    {
      typename ArrayType::Type v = typename ArrayType::Type(exp(inputs[0]->At(i) - maxValue));
      this->output_->At(i)       = v;
      sum += v;
    }
    for (size_t i(0); i < inputs[0]->size(); ++i)
    {
      this->output_->At(i) /= sum;
    }
    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs[0]->shape() == errorSignal->shape());

    ArrayPtrType t = this->Forward(inputs);
    for (size_t i(0); i < inputs[0]->size(); ++i)
    {
      errorSignal->At(i) *= t->At(i);
    }
    typename ArrayType::Type sum(0);
    for (size_t i(0); i < inputs[0]->size(); ++i)
    {
      sum += errorSignal->At(i);
    }
    for (size_t i(0); i < inputs[0]->size(); ++i)
    {
      errorSignal->At(i) -= (t->At(i) * sum);
    }
    return {errorSignal};
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
