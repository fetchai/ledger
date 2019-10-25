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

#include "ml/exceptions/exceptions.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MaxPool : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = typename TensorType::SizeType;
  using DataType      = typename TensorType::Type;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpMaxPool1DSaveableParams<T>;
  using MyType        = MaxPool<TensorType>;

  MaxPool(SizeType const kernel_size, SizeType const stride_size)
    : kernel_size_{kernel_size}
    , stride_size_{stride_size}
    , max_pool_1d_(kernel_size, stride_size)
    , max_pool_2d_(kernel_size, stride_size)
  {}

  explicit MaxPool(SPType const &sp)
    : Ops<T>(sp)
    , max_pool_1d_(sp.kernel_size, sp.stride_size)
    , max_pool_2d_(sp.kernel_size, sp.stride_size)
  {
    kernel_size_ = sp.kernel_size;
    stride_size_ = sp.stride_size;
  }

  MaxPool(MaxPool const &other) = default;

  ~MaxPool() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.kernel_size = kernel_size_;
    sp.stride_size = stride_size_;
    return std::make_shared<SPType>(sp);
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

    return copyshare;
  }
  /**
   * Applies 1D max pooling of kernel_size_ for each channel described here:
   * http://ais.uni-bonn.de/papers/icann2010_maxpool.pdf
   * @param inputs vector of tensor references where at:
   * inputs[0] = input_data[input_channels x input_height]
   * @param output tensor of size [input_channels=output_channels x number_of_stride_sized_steps]
   * @return: output tensor parameter
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);

    switch (inputs.at(0)->shape().size())
    {
    case 3:
    {
      max_pool_1d_.Forward(inputs, output);
      break;
    }
    case 2:
    {
      max_pool_2d_.Forward(inputs, output);
      break;
    }

    default:
    {
      throw fetch::ml::exceptions::InvalidMode("Unsupported data shape");
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
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);

    switch (inputs.at(0)->shape().size())
    {
    case 3:
    {
      return max_pool_1d_.Backward(inputs, error_signal);
      break;
    }
    case 2:
    {
      return max_pool_2d_.Backward(inputs, error_signal);
    }

    default:
    {
      throw fetch::ml::exceptions::InvalidMode("Unsupported data shape");
    }
    }
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    assert(inputs.size() == 1);

    switch (inputs.at(0)->shape().size())
    {
    case 3:
    {
      return max_pool_1d_.ComputeOutputShape(inputs);
      break;
    }
    case 2:
    {
      return max_pool_2d_.ComputeOutputShape(inputs);
    }

    default:
    {
      throw fetch::ml::exceptions::InvalidMode("Unsupported data shape");
    }
    }
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_MAX_POOL;
  }
  static constexpr char const *DESCRIPTOR = "MaxPool";

private:
  SizeType                              kernel_size_;
  SizeType                              stride_size_;
  fetch::ml::ops::MaxPool1D<TensorType> max_pool_1d_;
  fetch::ml::ops::MaxPool2D<TensorType> max_pool_2d_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
