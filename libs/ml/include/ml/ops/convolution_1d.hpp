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

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Convolution1D : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using DataType      = typename TensorType::Type;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpConvolution1DSaveableParams<TensorType>;
  using MyType        = Convolution1D<TensorType>;

  explicit Convolution1D(SizeType stride_size = 1)
    : stride_size_(stride_size)
  {}

  explicit Convolution1D(SPType const &sp)
    : Ops<T>(sp)
  {
    stride_size_ = sp.stride_size;
  }

  ~Convolution1D() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<typename TensorType::SizeType> ComputeOutputShape(
      VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_CONVOLUTION_1D;
  }
  static constexpr char const *DESCRIPTOR = "Convolution1D";

private:
  void FillVerticalStride(TensorType const &input, TensorType &vertical_stride,
                          SizeType output_channels, SizeType input_channels,
                          SizeType kernel_height);

  void ReverseFillVerticalStride(TensorType &input, TensorType const &vertical_stride,
                                 SizeType output_channels, SizeType input_channels,
                                 SizeType kernel_height);

  void FillHorizontalStride(TensorType const &input, TensorType &horizontal_stride,
                            SizeType output_height, SizeType input_channels, SizeType kernel_height,
                            SizeType batch_size);

  void ReverseFillHorizontalStride(TensorType &input, TensorType const &horizontal_stride,
                                   SizeType output_height, SizeType input_channels,
                                   SizeType kernel_height, SizeType batch_size);

  void FillOutput(TensorType const &gemm_output, TensorType &output, SizeType output_channels,
                  SizeType output_height, SizeType batch_size);

  void ReverseFillOutput(TensorType &gemm_output, TensorType const &output,
                         SizeType output_channels, SizeType output_height, SizeType batch_size);

  SizeType stride_size_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
