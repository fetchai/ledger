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

#include "math/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MatrixMultiply : public fetch::ml::BatchOps<T>
{
public:
  using ArrayType      = T;
  using SizeType       = typename ArrayType::SizeType;
  using ArrayPtrType   = std::shared_ptr<ArrayType>;
  using ConstSliceType = typename ArrayType::ConstSliceType;

  MatrixMultiply()          = default;
  virtual ~MatrixMultiply() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    (void)output;
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape().size() == 2);
    assert(inputs.at(1).get().shape().size() == 2);
    ASSERT(output.shape() == ComputeOutputShape(inputs));

    fetch::math::Dot(inputs[0].get(), inputs[1].get(), output);
    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 2);

    ArrayType errorSignal1(inputs.at(0).get().shape());
    ArrayType errorSignal2(inputs.at(1).get().shape());

    fetch::math::DotTranspose(errorSignal, inputs.at(1).get(), errorSignal1);
    fetch::math::TransposeDot(inputs.at(0).get(), errorSignal, errorSignal2);

    return {errorSignal1, errorSignal2};
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    return {inputs.at(0).get().shape()[0], inputs.at(1).get().shape()[1]};
  }

  static constexpr char const *DESCRIPTOR = "MatrixMultiply";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
