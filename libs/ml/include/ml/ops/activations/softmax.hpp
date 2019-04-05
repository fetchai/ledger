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

#include "math/ml/activation_functions/softmax.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Softmax : public fetch::ml::BatchOps<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Softmax()          = default;
  virtual ~Softmax() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    ASSERT(output.shape() == ComputeOutputSize(inputs));
    assert(inputs.size() == 1);
    fetch::math::Softmax(inputs[0].get(), output);
    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == errorSignal.shape());

    ArrayType returnSignal = errorSignal.Clone();

    ArrayType t(this->ComputeOutputSize(inputs));
    t = this->Forward(inputs, t);
    returnSignal.InlineMultiply(t);
    typename ArrayType::Type sum = returnSignal.Sum();
    t.InlineMultiply(sum);
    returnSignal.InlineSubtract(t);
    return {returnSignal};
  }

    virtual std::vector<SizeType> ComputeOutputSize(
						  std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    return inputs.front().get().shape();
  }


  static constexpr char const *DESCRIPTOR = "Softmax";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
