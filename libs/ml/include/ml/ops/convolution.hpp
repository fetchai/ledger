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
class Convolution : public BatchOps<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Convolution()          = default;
  virtual ~Convolution() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 2);
    // Input should be a 3D tensor [C x H x W]
    ASSERT(inputs.at(0).get().shape().size() == 3);
    // Weights should be a 4D tensor [oC x iC x H x W]
    ASSERT(inputs.at(1).get().shape().size() == 4);

    auto outputShape = ComputeOutputShape(inputs);
    ASSERT(output.shape() == outputShape);
    for (uint64_t i(0); i < outputShape[0]; ++i)  // Iterate over output channels
    {
      for (SizeType j{0}; j < outputShape[1]; ++j)  // Iterate over output height
      {
        for (SizeType k{0}; k < outputShape[2]; ++k)  // Iterate over output width
        {
          typename ArrayType::Type sum(0);
          for (uint64_t ki(0); ki < inputs.at(1).get().shape()[1];
               ki++)  // Iterate over Input channel
          {
            for (uint64_t kj(0); kj < inputs.at(1).get().shape()[2];
                 kj++)  // Iterate over kernel height
            {
              for (uint64_t kk(0); kk < inputs.at(1).get().shape()[3];
                   kk++)  // Iterate over kernel width
              {
                typename ArrayType::Type w = inputs.at(1).get().At(i, ki, kj, kk);
                typename ArrayType::Type i = inputs.at(0).get().At(ki, j + kj, k + kk);
                sum += i * w;
              }
            }
          }
          output.Set(std::vector<typename ArrayType::SizeType>({i, j, k}), sum);
        }
      }
    }
    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const & /*inputs*/,
      ArrayType const &errorSignal)
  {
    return {errorSignal};
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    std::vector<typename ArrayType::SizeType> outputShape;
    outputShape.push_back(inputs.at(1).get().shape()[0]);
    outputShape.push_back(inputs.at(0).get().shape()[1] - inputs.at(1).get().shape()[2] + 1);
    outputShape.push_back(inputs.at(0).get().shape()[2] - inputs.at(1).get().shape()[3] + 1);
    return outputShape;
  }

  static constexpr char const *DESCRIPTOR = "Convolution";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
