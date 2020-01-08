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
class MatrixMultiply : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using SizeVector    = typename TensorType::SizeVector;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpMatrixMultiplySaveableParams<T>;
  using MyType        = MatrixMultiply<TensorType>;

  explicit MatrixMultiply(bool transpose_a = false, bool transpose_b = false)
    : transpose_a_(transpose_a)
    , transpose_b_(transpose_b)
  {}

  explicit MatrixMultiply(SPType const &sp);

  ~MatrixMultiply() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  /**
   * This op should not be shared because it uses cacheing, therefore MakeSharedCopy returns a new
   * op
   * @param me
   * @return
   */
  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);

    copyshare->error_signal_1_       = error_signal_1_.Copy();
    copyshare->error_signal_2_       = error_signal_2_.Copy();
    copyshare->output_view_tensor_   = output_view_tensor_.Copy();
    copyshare->fwd_in1_view_tensor_  = fwd_in1_view_tensor_.Copy();
    copyshare->fwd_in2_view_tensor_  = fwd_in2_view_tensor_.Copy();
    copyshare->back_in1_view_tensor_ = back_in1_view_tensor_.Copy();
    copyshare->back_in2_view_tensor_ = back_in2_view_tensor_.Copy();
    copyshare->err_sig_view_tensor_  = err_sig_view_tensor_.Copy();
    copyshare->err1_                 = err1_.Copy();
    copyshare->err2_                 = err2_.Copy();
    copyshare->transpose_a_          = transpose_a_;
    copyshare->transpose_b_          = transpose_b_;

    return copyshare;
  }

  void                    Forward(VecTensorType const &inputs, TensorType &output) override;
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;
  std::vector<SizeType>   ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_MATRIX_MULTIPLY;
  }
  static constexpr char const *DESCRIPTOR = "MatrixMultiply";

private:
  // caching tensors and shapes
  TensorType error_signal_1_;
  TensorType error_signal_2_;

  // forward pass
  SizeVector fwd_input_shape_1_{};
  SizeVector fwd_input_shape_2_{};
  TensorType output_view_tensor_;
  TensorType fwd_in1_view_tensor_;
  TensorType fwd_in2_view_tensor_;

  // backward pass
  SizeVector back_input_shape_1_{};
  SizeVector back_input_shape_2_{};
  TensorType back_in1_view_tensor_;
  TensorType back_in2_view_tensor_;
  TensorType err_sig_view_tensor_;
  TensorType err1_;
  TensorType err2_;

  bool transpose_a_ = false;
  bool transpose_b_ = false;

  void UpdateContainersForward(VecTensorType const &inputs);
  void UpdateContainersBackward(VecTensorType const &inputs, TensorType const &error_signal);
  void DotWithTranspose(TensorType const &a, TensorType const &b, TensorType &ret);
  void BackDotWithTranspose(TensorType const &a, TensorType const &b, TensorType const &err_signal,
                            TensorType &err_ret_1, TensorType &err_ret_2);
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
