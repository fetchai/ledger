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
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  MatrixMultiply()          = default;
  virtual ~MatrixMultiply() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    (void)output;
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape().size() == 2);
    assert(inputs.at(1).get().shape().size() == 2);

    // inner dimension check
    assert(inputs.at(0).get().shape()[1] == inputs.at(1).get().shape()[0]);

    std::vector<SizeType> outputShape(ComputeOutputSize(inputs));
    if (!this->output_ || this->output_->shape() != outputShape)
    {
      this->output_ = std::make_shared<ArrayType>(outputShape);
    }

    for (SizeType i(0); i < inputs.at(0).get().shape()[0]; ++i)
    {
      for (SizeType j(0); j < inputs.at(1).get().shape()[1]; ++j)
      {
        this->output_->At(std::vector<SizeType>({i, j})) =
            inputs.at(0).get().At(std::vector<SizeType>({i, 0})) *
            inputs.at(1).get().At(std::vector<SizeType>({0, j}));
        for (SizeType k(1); k < inputs.at(0).get().shape()[1]; ++k)
        {
          this->output_->At(std::vector<SizeType>({i, j})) +=
              inputs.at(0).get().At(std::vector<SizeType>({i, k})) *
              inputs.at(1).get().At(std::vector<SizeType>({k, j}));
        }
      }
    }
    return *this->output_;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 2);

    ArrayType errorSignal1(inputs.at(0).get().shape());
    ArrayType errorSignal2(inputs.at(1).get().shape());

    fetch::math::DotTranspose(errorSignal, inputs.at(1).get(), errorSignal1);
    fetch::math::TransposeDot(inputs.at(0).get(), errorSignal, errorSignal2);

    return {errorSignal1, errorSignal2};
  }

  virtual std::vector<SizeType> ComputeOutputSize(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    return {inputs.at(0).get().shape()[0], inputs.at(1).get().shape()[1]};
  }

  static constexpr char const *DESCRIPTOR = "MatrixMultiply";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
