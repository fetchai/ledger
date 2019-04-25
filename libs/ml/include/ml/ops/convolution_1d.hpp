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

#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Convolution1D : public BatchOps<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Convolution1D()  = default;
  ~Convolution1D() = default;

  ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                    ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 2);
    // Input should be a 2D tensor [C x W]
    ASSERT(inputs.at(0).get().shape().size() == 2);
    // Weights should be a 3D tensor [oC x iC x  W]
    ASSERT(inputs.at(1).get().shape().size() == 3);

    auto outputShape = ComputeOutputShape(inputs);
    ASSERT(output.shape() == outputShape);
    for (uint64_t i(0); i < outputShape[0]; ++i)  // Iterate over output channels
    {
      for (SizeType j{0}; j < outputShape[1]; ++j)  // Iterate over output width
      {

        typename ArrayType::Type sum(0);
        for (uint64_t ki(0); ki < inputs.at(1).get().shape()[1];
             ki++)  // Iterate over Input channel
        {
          for (uint64_t kj(0); kj < inputs.at(1).get().shape()[2];
               kj++)  // Iterate over kernel width
          {

            typename ArrayType::Type w = inputs.at(1).get().At(i, ki, kj);
            typename ArrayType::Type i = inputs.at(0).get().At(ki, j + kj);
            sum += i * w;
          }
        }
        output.Set(i, j, sum);
      }
    }
    return output;
  }

  std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const & /*inputs*/,
      ArrayType const &errorSignal)
  {
    return {errorSignal};
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    std::vector<typename ArrayType::SizeType> outputShape;
    outputShape.push_back(inputs.at(1).get().shape()[0]);
    outputShape.push_back(inputs.at(0).get().shape()[1] - inputs.at(1).get().shape()[2] + 1);
    return outputShape;
  }

  static constexpr char const *DESCRIPTOR = "Convolution1D";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
