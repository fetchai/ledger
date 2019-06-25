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
#include "math/tensor.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MatrixMultiply : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  MatrixMultiply()  = default;
  ~MatrixMultiply() = default;

  void                   Forward(VecTensorType const &inputs, ArrayType &output);
  std::vector<ArrayType> Backward(VecTensorType const &inputs, ArrayType const &error_signal);
  std::vector<SizeType>  ComputeOutputShape(VecTensorType const &inputs) const;

  static constexpr char const *DESCRIPTOR = "MatrixMultiply";
};

template <class T>
void MatrixMultiply<T>::Forward(VecTensorType const &inputs, ArrayType &output)
{
  assert(inputs.size() == 2);
  assert(output.shape() == ComputeOutputShape(inputs));

  // Normal MatMul 2D @ 2D
  if (inputs.at(0).get().shape().size() == 2 && inputs.at(1).get().shape().size() == 2)
  {
    assert((inputs.at(0).get().shape().size() == 2 && inputs.at(1).get().shape().size() == 2));
    fetch::math::Dot(inputs.at(0).get(), inputs.at(1).get(), output);
  }
  // Batchwise 3D @ 3D or broadcast matmul 2D @ 3D, 3D @ 2D
  else
  {
    assert((inputs.at(0).get().shape().size() == 3 || inputs.at(0).get().shape().size() == 2) &&
           (inputs.at(1).get().shape().size() == 3 || inputs.at(1).get().shape().size() == 2));

    // Get batch size
    SizeType batch_size;
    if (inputs.at(0).get().shape().size() == 3)
    {
      batch_size = inputs.at(0).get().shape().at(2);
    }
    else
    {
      batch_size = inputs.at(1).get().shape().at(2);
    }

    // Iterate over batch
    for (SizeType i{0}; i < batch_size; i++)
    {

      auto      output_slice        = output.Slice(i, 2);
      ArrayType output_slice_tensor = output_slice.Copy();
      output_slice_tensor.Reshape(
          {output_slice_tensor.shape().at(0), output_slice_tensor.shape().at(1)});

      ArrayType in1_slice;
      // 3D @ ? case
      if (inputs.at(0).get().shape().size() == 3)
      {
        in1_slice = inputs.at(0).get().Slice(i, 2).Copy();
        in1_slice.Reshape({in1_slice.shape().at(0), in1_slice.shape().at(1)});
        // 2D @ 3D case
      }
      else
      {
        in1_slice = inputs.at(0).get();
      }

      ArrayType in2_slice;
      // ? @ 3D case
      if (inputs.at(1).get().shape().size() == 3)
      {
        in2_slice = inputs.at(1).get().Slice(i, 2).Copy();
        in2_slice.Reshape({in2_slice.shape().at(0), in2_slice.shape().at(1)});
      }
      // 3D @ 2D case
      else
      {
        in2_slice = inputs.at(1).get();
      }

      fetch::math::Dot(in1_slice, in2_slice, output_slice_tensor);

      // Copy data to original array
      output_slice.Assign(output_slice_tensor);
    }
  }
}

template <class T>
std::vector<T> MatrixMultiply<T>::Backward(VecTensorType const &inputs,
                                           ArrayType const &    error_signal)
{
  assert(inputs.size() == 2);

  ArrayType error_signal1(inputs.at(0).get().shape());
  ArrayType error_signal2(inputs.at(1).get().shape());

  // Normal MatMul 2D @ 2D
  if (inputs.at(0).get().shape().size() == 2 && inputs.at(1).get().shape().size() == 2)
  {
    fetch::math::DotTranspose(error_signal, inputs.at(1).get(), error_signal1);
    fetch::math::TransposeDot(inputs.at(0).get(), error_signal, error_signal2);
  }
  // Batchwise 3D @ 3D or broadcast matmul 2D @ 3D, 3D @ 2D
  else
  {
    assert((inputs.at(0).get().shape().size() == 3 || inputs.at(0).get().shape().size() == 2) &&
           (inputs.at(1).get().shape().size() == 3 || inputs.at(1).get().shape().size() == 2));

    // Get batch size
    SizeType batch_size;
    if (inputs.at(0).get().shape().size() == 3)
    {
      batch_size = inputs.at(0).get().shape().at(2);
    }
    else
    {
      batch_size = inputs.at(1).get().shape().at(2);
    }

    // Iterate over batch
    for (SizeType i{0}; i < batch_size; i++)
    {
      ArrayType err_sig_slice = error_signal.Slice(i, 2).Copy();
      ArrayType err1(error_signal1.shape());
      ArrayType err2(error_signal2.shape());

      ///////////////////////////////
      /// PREPARE DATA FOR MATMUL ///
      ///////////////////////////////

      ArrayType in1_slice;
      // 3D @ ? case
      if (inputs.at(0).get().shape().size() == 3)
      {
        in1_slice = inputs.at(0).get().Slice(i, 2).Copy().Squeeze();
      }
      // 2D @ 3D case
      else
      {
        in1_slice = inputs.at(0).get();
      }

      ArrayType in2_slice;
      // ? @ 3D case
      if (inputs.at(1).get().shape().size() == 3)
      {
        in2_slice = inputs.at(1).get().Slice(i, 2).Copy().Squeeze();
      }
      // 3D @ 2D case
      else
      {
        in2_slice = inputs.at(1).get();
      }

      /////////////////
      /// DO MATMUL ///
      /////////////////

      fetch::math::DotTranspose(err_sig_slice, in2_slice, err1);
      fetch::math::TransposeDot(in1_slice, err_sig_slice, err2);

      ////////////////////////////////
      /// COPY DATA BACK TO SLICES ///
      ////////////////////////////////

      // Copy data to original array
      // 3D @ ? case
      if (inputs.at(0).get().shape().size() == 3)
      {
        auto err1_slice = error_signal1.Slice(i, 2);
        err1_slice.Assign(err1);
      }
      // 2D @ 3D case
      else
      {
        fetch::math::Add(error_signal1, err1, error_signal1);
      }

      // Copy data to original array
      // ? @ 3D case
      if (inputs.at(1).get().shape().size() == 3)
      {
        auto err2_slice = error_signal2.Slice(i, 2);
        err2_slice.Assign(err2);
      }
      // 3D @ 2D case
      else
      {
        fetch::math::Add(error_signal2, err2, error_signal2);
      }
    }
  }

  return {error_signal1, error_signal2};
}

template <class T>
std::vector<typename T::SizeType> MatrixMultiply<T>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  // Normal Matmul
  if (inputs.at(0).get().shape().size() == 2 && inputs.at(1).get().shape().size() == 2)
  {
    return {inputs.at(0).get().shape().at(0), inputs.at(1).get().shape().at(1)};
  }
  // Batchwise matmul or 3D @ 2D broadcast matmul
  else if (inputs.at(0).get().shape().size() == 3)
  {
    return {inputs.at(0).get().shape().at(0), inputs.at(1).get().shape().at(1),
            inputs.at(0).get().shape().at(2)};
  }
  // 2D @ 3D broadcast matmul
  else
  {
    return {inputs.at(0).get().shape().at(0), inputs.at(1).get().shape().at(1),
            inputs.at(1).get().shape().at(2)};
  }
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
