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

#include "math/free_functions/ml/activation_functions/softmax.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Softmax : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Softmax()          = default;
  virtual ~Softmax() = default;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 1);
    if (!this->output_ || this->output_->shape() != inputs[0]->shape())
    {
      this->output_ = std::make_shared<ArrayType>(inputs[0]->shape());
    }
    fetch::math::Softmax(*inputs[0], *this->output_);
    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs[0]->shape() == errorSignal->shape());

    // Indicate if we're running in batch mode
    bool unsqueeze = (inputs[0]->shape().size() == 1);
    if (unsqueeze)
    {
      inputs[0]->Unsqueeze();
      errorSignal->Unsqueeze();
    }

    ArrayPtrType t = this->Forward(inputs);
    errorSignal->InlineMultiply(*t);
    for (uint64_t b(0); b < inputs[0]->shape()[0]; ++b)
    {
      ArrayType                inputBatchItem       = inputs[0]->Slice(b);
      ArrayType                errorSignalBatchItem = errorSignal->Slice(b);
      ArrayType                tBatchItem           = t->Slice(b);
      typename ArrayType::Type sum                  = errorSignalBatchItem.Sum();
      for (std::size_t i(0); i < inputBatchItem.size(); ++i)
      {
        errorSignalBatchItem.At(i) -= (tBatchItem.At(i) * sum);
      }
    }

    // Go back to input data format
    if (unsqueeze)
    {
      inputs[0]->Squeeze();
      errorSignal->Squeeze();
    }
    return {errorSignal};
  }

  static constexpr char const *DESCRIPTOR = "Softmax";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
