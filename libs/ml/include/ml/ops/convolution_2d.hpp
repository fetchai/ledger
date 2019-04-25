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
class Convolution2D : public BatchOps<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Convolution2D()  = default;
  ~Convolution2D() = default;

  ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                    ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 2);
    // Input should be a 3D tensor [C x H x W]
    ASSERT(inputs.at(0).get().shape().size() == 3);
    // Weights should be a 4D tensor [oC x iC x H x W]
    ASSERT(inputs.at(1).get().shape().size() == 4);

    auto outputShape = ComputeOutputShape(inputs);
    ASSERT(output.shape() == outputShape);

    SizeType input_channels  = inputs.at(0).get().shape().at(0);
    SizeType output_channels = inputs.at(1).get().shape().at(0);
    SizeType kernel_height   = inputs.at(1).get().shape().at(2);
    SizeType kernel_width    = inputs.at(1).get().shape().at(3);
    SizeType output_height   = outputShape.at(1);
    SizeType output_width    = outputShape.at(2);

    SizeType horizontal_stride_width  = kernel_width * kernel_height * input_channels;
    SizeType horizontal_stride_height = output_height * output_width;
    SizeType vertical_stride_width    = output_channels;

    ArrayType horizontal_stride({horizontal_stride_width, horizontal_stride_height});
    ArrayType vertical_stride({vertical_stride_width, horizontal_stride_width});

    // Fill horizontal stride
    SizeType i_s;  // stride width iterator
    SizeType j_s;  // stride height iterator

    j_s = 0;
    for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
    {
      for (SizeType j_o{0}; j_o < output_width; ++j_o)  // Iterate over output width
      {
        i_s = 0;
        for (SizeType i_ic(0); i_ic < input_channels; ++i_ic)  // Iterate over input channels
        {

          for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
          {
            for (SizeType j_k(0); j_k < kernel_width; j_k++)  // Iterate over kernel width
            {

              horizontal_stride.Set(i_s, j_s, inputs.at(0).get().At(i_ic, i_o + i_k, j_o + j_k));
              ++i_s;
            }
          }
        }
        ++j_s;
      }
    }

    // Fill vertical stride
    for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
    {
      j_s = 0;
      for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)  // Iterate over input channels
      {

        for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
        {
          for (SizeType j_k(0); j_k < kernel_width; j_k++)  // Iterate over kernel width
          {

            vertical_stride.Set(i_oc, j_s, inputs.at(1).get().At(i_oc, i_ic, i_k, j_k));
            ++j_s;
          }
        }
      }
    }

    // Do matmul
    ArrayType tmp_output = fetch::math::Dot(vertical_stride, horizontal_stride);

    // Put values to output
    SizeType i_tmp = 0;
    for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
    {
      i_tmp = 0;
      for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
      {
        for (SizeType j_o{0}; j_o < output_width; ++j_o)  // Iterate over output width
        {
          output.Set(i_oc, i_o, j_o, tmp_output.At(i_oc, i_tmp));
          ++i_tmp;
        }
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
    outputShape.push_back(inputs.at(0).get().shape()[2] - inputs.at(1).get().shape()[3] + 1);
    return outputShape;
  }

  static constexpr char const *DESCRIPTOR = "Convolution2D";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
