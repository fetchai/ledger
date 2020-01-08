#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
  using SizeType      = fetch::math::SizeType;
  using DataType      = typename TensorType::Type;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpMaxPoolSaveableParams<T>;
  using MyType        = MaxPool<TensorType>;

  MaxPool(SizeType const kernel_size, SizeType const stride_size)
    : kernel_size_{kernel_size}
    , stride_size_{stride_size}
  {}

  explicit MaxPool(SPType const &sp);

  MaxPool(MaxPool const &other) = default;

  ~MaxPool() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

  /**
   * Applies 1D/2D max pooling of kernel_size_ (x kernel_size_) for each channel described here:
   * http://ais.uni-bonn.de/papers/icann2010_maxpool.pdf
   * @param inputs vector of tensor references where at:
   * inputs[0] = input_data[input_channels x input_height (x input_width)]
   * @param output tensor of size [input_channels=output_channels x
   * number_of_stride_sized_steps_over_input_height (x
   * number_of_stride_sized_steps_over_input_width)]
   * @return: output tensor parameter
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override;

  /**
   * Computes gradient of 2D max pooling of kernel_size_ (x kernel_size) for each channel described
   * here: http://ais.uni-bonn.de/papers/icann2010_maxpool.pdf Error signal
   * of max pool is passed only to max node
   * @param inputs vector of tensor references where at:
   * inputs[0] = input_data[input_channels x input_height (x input_width)]
   * @param error_signal tensor of size  [output_channels x
   * number_of_stride_sized_steps_over_input_height (x
   * number_of_stride_sized_steps_over_input_width)]
   * @return: output vector of tensors with back propagated error signal
   * output[0]=input_error[inputs[0].shape]
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_MAX_POOL;
  }
  static constexpr char const *DESCRIPTOR = "MaxPool";

private:
  SizeType                                         kernel_size_;
  SizeType                                         stride_size_;
  bool                                             pool_2d_;
  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> pool_op_ptr_;

  void UpdatePointer(VecTensorType const &inputs);
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
