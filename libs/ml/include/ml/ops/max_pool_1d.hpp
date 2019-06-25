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
class MaxPool1D : public Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using DataType      = typename ArrayType::Type;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  MaxPool1D(SizeType const kernel_size, SizeType const stride_size)
    : kernel_size_{kernel_size}
    , stride_size_{stride_size}
  {}

  ~MaxPool1D() = default;

  /**
   * Applies 1D max pooling of kernel_size_ for each channel described here:
   * http://ais.uni-bonn.de/papers/icann2010_maxpool.pdf
   * @param inputs vector of tensor references where at:
   * inputs[0] = input_data[input_channels x input_height]
   * @param output tensor of size [input_channels=output_channels x number_of_stride_sized_steps]
   * @return: output tensor parameter
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    ASSERT(inputs.size() == 1);
    // Input must be a 3D tensor [C x W x N]
    ASSERT(inputs.at(0).get().shape().size() == 3);
    ASSERT(output.shape() == ComputeOutputShape(inputs));

    SizeType iter;
    DataType max;
    DataType val;
    auto     out_it = output.begin();

    for (SizeType n_i{0}; n_i < output.shape().at(2); n_i++)  // iterate over batch
    {
      // output_channels = input_channels
      for (SizeType i{0}; i < output.shape().at(1); i++)  // Iterate over kernel stride
      {
        iter = i * stride_size_;
        for (SizeType c{0}; c < output.shape().at(0); ++c)  // Iterate over output channels
        {
          max = inputs.at(0).get().At(c, iter, n_i);

          // Get maximum value on kernel_size_ window
          for (SizeType j{1}; j < kernel_size_; j++)  // Iterate over kernel width
          {
            val = inputs.at(0).get().At(c, iter + j, n_i);
            if (val > max)
            {
              max = val;
            }
          }

          // Set maximum value for each [kernel_size_] window to output
          *out_it = max;
          ++out_it;
        }
      }
    }
  }

  /**
   * Computes gradient of 1D max pooling of kernel_size_ for each channel described here:
   * http://ais.uni-bonn.de/papers/icann2010_maxpool.pdf
   * Error signal of max pool is passed only to max node
   * @param inputs vector of tensor references where at:
   * inputs[0] = input_data[input_channels x input_height]
   * @param error_signal tensor of size [output_channels=input_channels x
   * number_of_stride_sized_steps]
   * @return: output vector of tensors with back propagated error signal
   * output[0]=input_error[inputs[0].shape]
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    ASSERT(inputs.size() == 1);
    ASSERT(error_signal.shape() == ComputeOutputShape(inputs));

    ArrayType return_signal{inputs.at(0).get().shape()};

    auto output_shape = error_signal.shape();

    SizeType iter;
    DataType max;
    DataType val;
    SizeType max_iter;
    auto     er_it = error_signal.cbegin();

    for (SizeType n_i{0}; n_i < output_shape.at(2); n_i++)  // iterate over batch
    {
      for (SizeType i{0}; i < output_shape.at(1); i++)  // Iterate over kernel stride
      {
        iter = i * stride_size_;
        for (SizeType c{0}; c < output_shape.at(0); ++c)  // Iterate over output channels
        {
          max      = inputs.at(0).get().At(c, iter, n_i);
          max_iter = iter;

          // Find max node
          for (SizeType j{0}; j < kernel_size_; j++)  // Iterate over kernel width
          {
            val = inputs.at(0).get().At(c, iter + j, n_i);
            if (val > max)
            {
              max      = val;
              max_iter = iter + j;
            }
          }

          // Add error to max node
          return_signal.Set(c, max_iter, n_i, return_signal.At(c, max_iter, n_i) + *er_it);
          ++er_it;
        }
      }
    }

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    std::vector<SizeType> output_shape;

    // output_shape_[0]=number of output channels
    output_shape.emplace_back(inputs.at(0).get().shape().at(0));
    // output_shape_[1]=number of stride_size steps over input size
    output_shape.emplace_back((inputs.at(0).get().shape().at(1) - (kernel_size_ - stride_size_)) /
                              stride_size_);
    // output_shape_[2]=batch dimension
    output_shape.emplace_back(inputs.at(0).get().shape().at(2));
    return output_shape;
  }

  static constexpr char const *DESCRIPTOR = "MaxPool1D";

private:
  SizeType kernel_size_;
  SizeType stride_size_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
