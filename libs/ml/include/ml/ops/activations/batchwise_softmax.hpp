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

#include "math/ml/activation_functions/softmax.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class BatchwiseSoftmax : public fetch::ml::BatchOps<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename ElementWiseOps<T>::VecTensorType;

  BatchwiseSoftmax(SizeType axis = 1)
    : axis_(axis)
  {}

  ~BatchwiseSoftmax() = default;

  void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(output.shape() == ComputeOutputShape(inputs));
    assert(inputs.size() == 1);

    SizeType batch_dimension = inputs.at(0).get().shape().size() - 1;

    for (SizeType i{0}; i < inputs.at(0).get().shape().at(batch_dimension); i++)
    {

      ArrayType input_slice_tensor  = inputs.at(0).get().Slice(i, batch_dimension).Copy();
      auto      output_slice        = output.Slice(i, batch_dimension);
      ArrayType output_slice_tensor = output_slice.Copy();

      input_slice_tensor.Squeeze();
      output_slice_tensor.Squeeze();

      fetch::math::Softmax(input_slice_tensor, output_slice_tensor, axis_);

      auto output_slice_it        = output_slice.begin();
      auto output_slice_tensor_it = output_slice_tensor.begin();

      while (output_slice_tensor_it.is_valid())
      {
        *output_slice_it = *output_slice_tensor_it;
        ++output_slice_it;
        ++output_slice_tensor_it;
      }
    }
  }

  ArrayType BackwardSlice(ArrayType const &input, ArrayType const &error_signal)
  {
    ArrayType return_signal = error_signal.Copy();
    ArrayType t(error_signal.shape());
    fetch::math::Softmax(input, t, axis_);
    return_signal.InlineMultiply(t);

    // 1D softmax
    if (input.shape().size() == 1)
    {
      typename ArrayType::Type sum = return_signal.Sum();
      t.InlineMultiply(sum);
    }
    // 2D softmax
    else if (input.shape().size() == 2)
    {
      ArrayType sum;
      sum = ReduceSum(return_signal, 1 - axis_);

      t.InlineMultiply(sum);
    }
    else
    {
      throw std::runtime_error("Softmax over >= 3 dimensions not implemented");
    }

    return_signal.InlineSubtract(t);
    return return_signal;
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs, ArrayType const &error_signal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == error_signal.shape());

    SizeType batch_dimension = inputs.front().get().shape().size() - 1;
    SizeType batch_size      = inputs.front().get().shape().at(batch_dimension);

    ArrayType return_signal = error_signal.Copy();

    for (SizeType i{0}; i < batch_size; i++)
    {
      ArrayType input_slice        = inputs.front().get().Slice(i, batch_dimension).Copy();
      ArrayType error_signal_slice = error_signal.Slice(i, batch_dimension).Copy();

      input_slice.Squeeze();
      error_signal_slice.Squeeze();

      ArrayType slice_return_signal = BackwardSlice(input_slice, error_signal_slice);

      auto return_signal_slice    = return_signal.Slice(i, batch_dimension);
      auto return_signal_slice_it = return_signal_slice.begin();
      auto slice_return_signal_it = slice_return_signal.begin();
      while (return_signal_slice_it.is_valid())
      {
        *return_signal_slice_it = *slice_return_signal_it;
        ++slice_return_signal_it;
        ++return_signal_slice_it;
      }
    }

    /*
    for(SizeType i{0};i<return_signal.shape().at(2);i++)
    {
    //std::cout<<"BatchSoftmaxRet("<<i<<")"<<std::endl<<return_signal.Slice(i,2).Copy().Squeeze().ToString()<<std::endl;
        std::cout<<"BatchSoftmaxErr("<<i<<")"<<std::endl<<error_signal.Slice(i,2).Copy().Squeeze().ToString()<<std::endl;
    }
     */

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "BatchwiseSoftmax";

private:
  SizeType axis_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
