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

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MaxPool2D : public Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using DataType      = typename ArrayType::Type;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  MaxPool2D(SizeType const kernel_size, SizeType const stride_size)
    : kernel_size_{kernel_size}
    , stride_size_{stride_size}
  {}
  ~MaxPool2D() override = default;

  /**
   * Applies 2D max pooling of kernel_size_ x kernel_size_ for each channel described here:
   * http://ais.uni-bonn.de/papers/icann2010_maxpool.pdf
   * @param inputs vector of tensor references where at:
   * inputs[0] = input_data[input_channels x input_height x input_width]
   * @param output tensor of size [input_channels=output_channels x
   * number_of_stride_sized_steps_over_input_height x number_of_stride_sized_steps_over_input_width]
   * @return: output tensor parameter
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 1);
    // Input must be a 4D tensor [C x W x H x N]
    assert(inputs.at(0)->shape().size() == 4);
    assert(output.shape() == ComputeOutputShape(inputs));

    SizeType iterw;
    SizeType iterh;
    DataType val;
    DataType max;
    auto     oit = output.begin();

    for (SizeType n_i{0}; n_i < output.shape().at(2); n_i++)  // iterate over batch
    {
      for (SizeType ih{0}; ih < output.shape().at(2); ih++)  // Iterate height over kernel stride
      {
        iterh = ih * stride_size_;

        for (SizeType iw{0}; iw < output.shape().at(1); iw++)  // Iterate width over kernel stride
        {
          iterw = iw * stride_size_;

          for (SizeType c{0}; c < output.shape().at(0); ++c)  // Iterate over output channels
          {
            max = inputs.at(0).get().At(c, iterw, iterh, n_i);

            // Get maximum value on kernel_size_ x kernel_size_ window
            for (SizeType jw{0}; jw < kernel_size_; jw++)  // Iterate over kernel width
            {
              for (SizeType jh{0}; jh < kernel_size_; jh++)  // Iterate over kernel width
              {
                val = inputs.at(0).get().At(c, iterw + jw, iterh + jh, n_i);
                if (val > max)
                {
                  max = val;
                }
              }
            }

            // Set maximum value for each [kernel_size_ x kernel_size_] window to output
            *oit = max;
            ++oit;
          }
        }
      }
    }
  }

  /**
   * Computes gradient of 2D max pooling of kernel_size_ x kernel_size for each channel described
   * here: http://ais.uni-bonn.de/papers/icann2010_maxpool.pdf Error signal
   * of max pool is passed only to max node
   * @param inputs vector of tensor references where at:
   * inputs[0] = input_data[input_channels x input_height x input_width]
   * @param error_signal tensor of size  [output_channels x
   * number_of_stride_sized_steps_over_input_height x number_of_stride_sized_steps_over_input_width]
   * @return: output vector of tensors with back propagated error signal
   * output[0]=input_error[inputs[0].shape]
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(error_signal.shape() == ComputeOutputShape(inputs));
    ArrayType return_signal{inputs.at(0)->shape()};

    SizeType iterh;
    SizeType iterw;
    DataType max;
    DataType val;
    SizeType max_iterw;
    SizeType max_iterh;
    auto     erit = error_signal.cbegin();
    for (SizeType n_i{0}; n_i < error_signal.shape().at(2); n_i++)  // iterate over batch
    {
      // Iterate width over kernel stride
      for (SizeType iw{0}; iw < error_signal.shape().at(1); iw++)
      {
        iterw = iw * stride_size_;
        // Iterate height over kernel stride
        for (SizeType ih{0}; ih < error_signal.shape().at(2); ih++)
        {
          iterh = ih * stride_size_;
          // Iterate over output channels
          for (SizeType c{0}; c < error_signal.shape().at(0); ++c)
          {
            max       = inputs.at(0).get().At(c, iterw, iterh, n_i);
            max_iterw = iterw;
            max_iterh = iterh;

            // Find max node
            for (SizeType jw{0}; jw < kernel_size_; jw++)  // Iterate over kernel width
            {
              for (SizeType jh{0}; jh < kernel_size_; jh++)  // Iterate over kernel width
              {

                val = inputs.at(0).get().At(c, iterw + jw, iterh + jh, n_i);
                if (val > max)
                {
                  max       = val;
                  max_iterw = iterw + jw;
                  max_iterh = iterh + jh;
                }
              }
            }

            // Add error to max node
            return_signal(c, max_iterw, max_iterh, n_i) =
                return_signal(c, max_iterw, max_iterh, n_i) + *erit;
            ++erit;
          }
        }
      }
    }

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    std::vector<SizeType> output_shape;

    // output_shape_[0]=number of output channels
    output_shape.emplace_back(inputs.at(0)->shape().at(0));
    // output_shape_[1]=number of stride_size steps over input height
    output_shape.emplace_back((inputs.at(0)->shape().at(1) - (kernel_size_ - stride_size_)) /
                              stride_size_);
    // output_shape_[2]=number of stride_size steps over input width
    output_shape.emplace_back((inputs.at(0)->shape().at(2) - (kernel_size_ - stride_size_)) /
                              stride_size_);
    // output_shape_[3]=batch dimension
    output_shape.emplace_back(inputs.at(0)->shape().at(3));
    return output_shape;
  }

  static constexpr char const *DESCRIPTOR = "MaxPool2D";

private:
  SizeType kernel_size_;
  SizeType stride_size_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
