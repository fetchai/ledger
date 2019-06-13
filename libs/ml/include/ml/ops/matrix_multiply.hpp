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
class MatrixMultiply : public fetch::ml::BatchOps<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename BatchOps<T>::VecTensorType;

  MatrixMultiply()  = default;
  ~MatrixMultiply() = default;

  void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(output.shape() == ComputeOutputShape(inputs));

    // Normal MatMul
    if (inputs.at(0).get().shape().size() == 2)
    {
      ASSERT(inputs.at(1).get().shape().size() == 2);
      fetch::math::Dot(inputs[0].get(), inputs[1].get(), output);
    }
    // Batchwise MatMul
    else
    {
      ASSERT(inputs.at(0).get().shape().size() == 3);
      ASSERT(inputs.at(1).get().shape().size() == 3);
      ASSERT(inputs.at(1).get().shape().at(2) == inputs.at(0).get().shape().at(2));

      SizeType batch_size{inputs.at(0).get().shape().at(2)};
      for (SizeType i{0}; i < batch_size; i++)
      {
        // Slice along batch dimension
        ArrayType in1_slice           = inputs.at(0).get().Slice(i, 2).Copy();
        ArrayType in2_slice           = inputs.at(1).get().Slice(i, 2).Copy();
        auto      output_slice        = output.Slice(i, 2);
        ArrayType output_slice_tensor = output_slice.Copy();

        // Remove batch dimension from slice tensor
        in1_slice.Reshape({in1_slice.shape().at(0), in1_slice.shape().at(1)});
        in2_slice.Reshape({in2_slice.shape().at(0), in2_slice.shape().at(1)});
        output_slice_tensor.Reshape(
            {output_slice_tensor.shape().at(0), output_slice_tensor.shape().at(1)});

        fetch::math::Dot(in1_slice, in2_slice, output_slice_tensor);

        // Copy data to original array
        auto output_slice_it        = output_slice.begin();
        auto output_slice_tensor_it = output_slice_tensor.begin();
        while (output_slice_it.is_valid())
        {
          *output_slice_it = *output_slice_tensor_it;
          ++output_slice_it;
          ++output_slice_tensor_it;
        }
      }
    }
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs, ArrayType const &error_signal)
  {
    ASSERT(inputs.size() == 2);

    ArrayType error_signal1(inputs.at(0).get().shape());
    ArrayType error_signal2(inputs.at(1).get().shape());

    // Normal MatMul
    if (inputs.at(0).get().shape().size() == 2)
    {
      fetch::math::DotTranspose(error_signal, inputs.at(1).get(), error_signal1);
      fetch::math::TransposeDot(inputs.at(0).get(), error_signal, error_signal2);
    }
    // Batchwise MatMul
    else
    {

      SizeType batch_size{inputs.at(0).get().shape().at(2)};
      for (SizeType i{0}; i < batch_size; i++)
      {
        // Slice along batch dimension
        ArrayType in1_slice     = inputs.at(0).get().Slice(i, 2).Copy();
        ArrayType in2_slice     = inputs.at(1).get().Slice(i, 2).Copy();
        ArrayType err_sig_slice = error_signal.Slice(i, 2).Copy();

        auto      err1_slice        = error_signal1.Slice(i, 2);
        auto      err2_slice        = error_signal2.Slice(i, 2);
        ArrayType err1_slice_tensor = err1_slice.Copy();
        ArrayType err2_slice_tensor = err2_slice.Copy();

        // Remove batch dimension from slice tensor
        in1_slice.Reshape({in1_slice.shape().at(0), in1_slice.shape().at(1)});
        in2_slice.Reshape({in2_slice.shape().at(0), in2_slice.shape().at(1)});
        err1_slice_tensor.Reshape(
            {err1_slice_tensor.shape().at(0), err1_slice_tensor.shape().at(1)});
        err2_slice_tensor.Reshape(
            {err2_slice_tensor.shape().at(0), err2_slice_tensor.shape().at(1)});
        err_sig_slice.Reshape({err_sig_slice.shape().at(0), err_sig_slice.shape().at(1)});

        fetch::math::DotTranspose(err_sig_slice, in2_slice, err1_slice_tensor);
        fetch::math::TransposeDot(in1_slice, err_sig_slice, err2_slice_tensor);

        // Copy data to original array
        auto err1_slice_it        = err1_slice.begin();
        auto err1_slice_tensor_it = err1_slice_tensor.begin();
        while (err1_slice_it.is_valid())
        {
          *err1_slice_it = *err1_slice_tensor_it;
          ++err1_slice_it;
          ++err1_slice_tensor_it;
        }

        auto err2_slice_it        = err2_slice.begin();
        auto err2_slice_tensor_it = err2_slice_tensor.begin();
        while (err2_slice_it.is_valid())
        {
          *err2_slice_it = *err2_slice_tensor_it;
          ++err2_slice_it;
          ++err2_slice_tensor_it;
        }
      }
    }

    return {error_signal1, error_signal2};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    // Normal Matmul
    if (inputs.at(0).get().shape().size() == 2)
    {
      return {inputs.at(0).get().shape()[0], inputs.at(1).get().shape()[1]};
    }
    // Batchwise matmul
    else
    {
      return {inputs.at(0).get().shape()[0], inputs.at(1).get().shape()[1],
              inputs.at(0).get().shape()[2]};
    }
  }

  static constexpr char const *DESCRIPTOR = "MatrixMultiply";

private:
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
