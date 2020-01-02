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

#include "math/matrix_operations.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
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

  explicit MatrixMultiply(SPType const &sp)
    : Ops<T>(sp)
  {
    transpose_a_ = sp.transpose_a;
    transpose_b_ = sp.transpose_b;
  }

  ~MatrixMultiply() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto ret         = std::make_shared<SPType>();
    ret->transpose_a = transpose_a_;
    ret->transpose_b = transpose_b_;
    return ret;
  }

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

template <class T>
void MatrixMultiply<T>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(output.shape() == ComputeOutputShape(inputs));

  UpdateContainersForward(inputs);

  // Normal MatMul 2D @ 2D
  if (inputs.at(0)->shape().size() == 2 && inputs.at(1)->shape().size() == 2)
  {
    assert((inputs.at(0)->shape().size() == 2 && inputs.at(1)->shape().size() == 2));
    DotWithTranspose((*inputs.at(0)), (*inputs.at(1)), output);
  }
  // Batchwise 3D @ 3D or broadcast matmul 2D @ 3D, 3D @ 2D
  else
  {
    assert((inputs.at(0)->shape().size() == 3 || inputs.at(0)->shape().size() == 2) &&
           (inputs.at(1)->shape().size() == 3 || inputs.at(1)->shape().size() == 2));

    // Get batch size
    SizeType batch_size;
    if (inputs.at(0)->shape().size() == 3)
    {
      batch_size = inputs.at(0)->shape().at(2);
    }
    else
    {
      batch_size = inputs.at(1)->shape().at(2);
    }

    // Iterate over batch
    for (SizeType i{0}; i < batch_size; i++)
    {
      auto output_view = output.View(i);
      output_view_tensor_.Assign(output.View(i));

      // 3D @ ? case
      if (inputs.at(0)->shape().size() == 3)
      {
        fwd_in1_view_tensor_.Assign(inputs.at(0)->View(i));
        // 2D @ 3D case
      }
      else
      {
        fwd_in1_view_tensor_ = (*inputs.at(0));
      }

      // ? @ 3D case
      if (inputs.at(1)->shape().size() == 3)
      {
        fwd_in2_view_tensor_.Assign(inputs.at(1)->View(i));
      }
      // 3D @ 2D case
      else
      {
        fwd_in2_view_tensor_ = (*inputs.at(1));
      }

      DotWithTranspose(fwd_in1_view_tensor_, fwd_in2_view_tensor_, output_view_tensor_);

      // Copy data to original array
      output_view.Assign(output_view_tensor_);
    }
  }
}

template <class T>
std::vector<T> MatrixMultiply<T>::Backward(VecTensorType const &inputs,
                                           TensorType const &   error_signal)
{
  assert(inputs.size() == 2);

  // no change in shape - we can use cached shape
  UpdateContainersBackward(inputs, error_signal);

  // Normal MatMul 2D @ 2D
  if (inputs.at(0)->shape().size() == 2 && inputs.at(1)->shape().size() == 2)
  {
    BackDotWithTranspose((*inputs.at(0)), (*inputs.at(1)), error_signal, error_signal_1_,
                         error_signal_2_);
  }
  // Batchwise 3D @ 3D or broadcast matmul 2D @ 3D, 3D @ 2D
  else
  {
    assert((inputs.at(0)->shape().size() == 3 || inputs.at(0)->shape().size() == 2) &&
           (inputs.at(1)->shape().size() == 3 || inputs.at(1)->shape().size() == 2));

    // Get batch size
    SizeType batch_size;
    if (inputs.at(0)->shape().size() == 3)
    {
      batch_size = inputs.at(0)->shape().at(2);
    }
    else
    {
      batch_size = inputs.at(1)->shape().at(2);
    }

    // Iterate over batch
    for (SizeType i{0}; i < batch_size; i++)
    {
      err_sig_view_tensor_.Assign(error_signal.View(i));

      ///////////////////////////////
      /// PREPARE DATA FOR MATMUL ///
      ///////////////////////////////

      // 3D @ ? case
      if (inputs.at(0)->shape().size() == 3)
      {
        back_in1_view_tensor_.Assign(inputs.at(0)->View(i));
      }
      // 2D @ 3D case
      else
      {
        back_in1_view_tensor_ = (*inputs.at(0));
      }

      // ? @ 3D case
      if (inputs.at(1)->shape().size() == 3)
      {
        back_in2_view_tensor_.Assign(inputs.at(1)->View(i));
      }
      // 3D @ 2D case
      else
      {
        back_in2_view_tensor_ = (*inputs.at(1));
      }

      /////////////////
      /// DO MATMUL ///
      /////////////////

      BackDotWithTranspose(back_in1_view_tensor_, back_in2_view_tensor_, err_sig_view_tensor_,
                           err1_, err2_);

      ////////////////////////////////
      /// COPY DATA BACK TO Views ///
      ////////////////////////////////

      // Copy data to original array
      // 3D @ ? case
      if (inputs.at(0)->shape().size() == 3)
      {
        auto err1_view = error_signal_1_.View(i);
        err1_view.Assign(err1_);
      }
      // 2D @ 3D case
      else
      {
        fetch::math::Add(error_signal_1_, err1_, error_signal_1_);
      }

      // Copy data to original array
      // ? @ 3D case
      if (inputs.at(1)->shape().size() == 3)
      {
        auto err2_view = error_signal_2_.View(i);
        err2_view.Assign(err2_);
      }
      // 3D @ 2D case
      else
      {
        fetch::math::Add(error_signal_2_, err2_, error_signal_2_);
      }
    }
  }

  return {error_signal_1_, error_signal_2_};
}

template <class T>
std::vector<typename fetch::math::SizeType> MatrixMultiply<T>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  if (transpose_a_ && transpose_b_)
  {
    throw ml::exceptions::InvalidMode(
        "ops::MatrixMultiply does not support both inputs transposed");
  }
  std::vector<fetch::math::SizeType> output_shape;

  // Normal Matmul
  if (inputs.at(0)->shape().size() == 2 && inputs.at(1)->shape().size() == 2)
  {
    if (!transpose_a_ && !transpose_b_)
    {
      output_shape = {inputs.at(0)->shape().at(0), inputs.at(1)->shape().at(1)};
    }
    else if (transpose_a_ & !transpose_b_)
    {
      output_shape = {inputs.at(0)->shape().at(1), inputs.at(1)->shape().at(1)};
    }
    else
    {
      output_shape = {inputs.at(0)->shape().at(0), inputs.at(1)->shape().at(0)};
    }
  }
  // Batchwise matmul or 3D @ 2D broadcast matmul
  else if (inputs.at(0)->shape().size() == 3)
  {
    if (!transpose_a_ && !transpose_b_)
    {
      output_shape = {inputs.at(0)->shape().at(0), inputs.at(1)->shape().at(1),
                      inputs.at(0)->shape().at(2)};
    }
    else if (transpose_a_ & !transpose_b_)
    {
      output_shape = {inputs.at(0)->shape().at(1), inputs.at(1)->shape().at(1),
                      inputs.at(0)->shape().at(2)};
    }
    else
    {
      output_shape = {inputs.at(0)->shape().at(0), inputs.at(1)->shape().at(0),
                      inputs.at(0)->shape().at(2)};
    }
  }
  else
  {
    // 2D @ 3D broadcast matmul
    if (!transpose_a_ && !transpose_b_)
    {
      output_shape = {inputs.at(0)->shape().at(0), inputs.at(1)->shape().at(1),
                      inputs.at(1)->shape().at(2)};
    }
    else if (transpose_a_ & !transpose_b_)
    {
      output_shape = {inputs.at(0)->shape().at(1), inputs.at(1)->shape().at(1),
                      inputs.at(1)->shape().at(2)};
    }
    else
    {
      output_shape = {inputs.at(0)->shape().at(0), inputs.at(1)->shape().at(0),
                      inputs.at(1)->shape().at(2)};
    }
  }

  return output_shape;
}

/**
 * Updates temporary container objects used in some cases of batched forward pass
 * @tparam T tensor type
 * @param inputs input tensors
 * @param output output tensor
 */
template <typename T>
void MatrixMultiply<T>::UpdateContainersForward(VecTensorType const &inputs)
{
  if (!((inputs.at(0)->shape() == fwd_input_shape_1_) &&
        (inputs.at(1)->shape() == fwd_input_shape_2_)))
  {
    fwd_input_shape_1_   = inputs.at(0)->shape();
    fwd_input_shape_2_   = inputs.at(1)->shape();
    fwd_in1_view_tensor_ = TensorType({inputs.at(0)->shape().at(0), inputs.at(0)->shape().at(1)});
    fwd_in2_view_tensor_ = TensorType({inputs.at(1)->shape().at(0), inputs.at(1)->shape().at(1)});

    if (!transpose_a_ && !transpose_b_)
    {
      output_view_tensor_ = TensorType({inputs.at(0)->shape().at(0), inputs.at(1)->shape().at(1)});
    }
    else if (transpose_a_ & !transpose_b_)
    {
      output_view_tensor_ = TensorType({inputs.at(0)->shape().at(1), inputs.at(1)->shape().at(1)});
    }
    else if (transpose_b_ & !transpose_a_)
    {
      output_view_tensor_ = TensorType({inputs.at(0)->shape().at(0), inputs.at(1)->shape().at(0)});
    }
    else
    {
      throw ml::exceptions::InvalidMode(
          "ops::MatrixMultiply does not support both inputs transposed");
    }
  }
}

/**
 * Updates temporary container objects used in batched back pass
 * @tparam T tensor type
 * @param inputs input tensors
 * @param error_signal back pass error signal
 */
template <typename T>
void MatrixMultiply<T>::UpdateContainersBackward(VecTensorType const &inputs,
                                                 TensorType const &   error_signal)
{
  if (!((inputs.at(0)->shape() == back_input_shape_1_) &&
        (inputs.at(1)->shape() == back_input_shape_2_)))
  {
    back_input_shape_1_ = inputs.at(0)->shape();
    back_input_shape_2_ = inputs.at(1)->shape();

    back_in1_view_tensor_ = TensorType({inputs.at(0)->shape().at(0), inputs.at(0)->shape().at(1)});
    back_in2_view_tensor_ = TensorType({inputs.at(1)->shape().at(0), inputs.at(1)->shape().at(1)});

    err1_                = TensorType(error_signal_1_.shape());
    err2_                = TensorType(error_signal_2_.shape());
    error_signal_1_      = TensorType(back_input_shape_1_);
    error_signal_2_      = TensorType(back_input_shape_2_);
    err_sig_view_tensor_ = TensorType({error_signal.shape().at(0), error_signal.shape().at(1)});
  }
}

/**
 * Invokes the relevant DotProduct operation depending on the transpose mode
 * @tparam TensorType
 * @param a
 * @param b
 * @param ret
 */
template <typename TensorType>
void MatrixMultiply<TensorType>::DotWithTranspose(TensorType const &a, TensorType const &b,
                                                  TensorType &ret)
{
  if (!transpose_a_ && !transpose_b_)
  {
    fetch::math::Dot(a, b, ret);
  }
  else if (transpose_a_ & !transpose_b_)
  {
    fetch::math::TransposeDot(a, b, ret);
  }
  else if (transpose_b_ & !transpose_a_)
  {
    fetch::math::DotTranspose(a, b, ret);
  }
  else
  {
    throw ml::exceptions::InvalidMode(
        "ops::MatrixMultiply does not support both inputs transposed");
  }
}

/**
 * Applies the relevant two dot operations in backprop depending on transpose
 * @tparam TensorType
 * @param a
 * @param b
 * @param ret
 */
template <typename TensorType>
void MatrixMultiply<TensorType>::BackDotWithTranspose(TensorType const &a, TensorType const &b,
                                                      TensorType const &err_signal,
                                                      TensorType &err_ret_1, TensorType &err_ret_2)
{

  if (!transpose_a_ && !transpose_b_)
  {
    fetch::math::DotTranspose(err_signal, b, err_ret_1);
    fetch::math::TransposeDot(a, err_signal, err_ret_2);
  }
  else if (transpose_a_ && !transpose_b_)
  {
    fetch::math::DotTranspose(err_signal, b, err_ret_1);
    fetch::math::Dot(a, err_signal, err_ret_2);
  }
  else if (transpose_b_ && !transpose_a_)
  {
    fetch::math::Dot(err_signal, b, err_ret_1);
    fetch::math::TransposeDot(a, err_signal, err_ret_2);
  }
  else
  {
    throw ml::exceptions::InvalidMode(
        "ops::MatrixMultiply does not support both inputs transposed");
  }
}
}  // namespace ops
}  // namespace ml
}  // namespace fetch
