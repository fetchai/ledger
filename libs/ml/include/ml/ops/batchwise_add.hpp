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
class BatchwiseAdd : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SizeType      = typename ArrayType::SizeType;

  BatchwiseAdd()          = default;
  virtual ~BatchwiseAdd() = default;

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape().at(0) == inputs.at(1).get().shape().at(0));
    assert(output.shape() == this->ComputeOutputShape(inputs));

    ArrayType A = inputs.at(0).get();
    ArrayType B = inputs.at(1).get();

    // Test if input is broadcastable by batch dimension
    assert(A.shape().size() == B.shape().size());
    assert(A.shape().size() == output.shape().size());
    assert(B.shape().at(B.shape().size() - 1) == 1);

    for (SizeType i{0}; i < A.shape().size() - 1; i++)
    {
      assert(A.shape().at(i) == B.shape().at(i));
      assert(A.shape().at(i) == output.shape().at(i));
    }

    SizeType A_batch_dimension      = A.shape().size() - 1;
    SizeType B_batch_dimension      = B.shape().size() - 1;
    SizeType output_batch_dimension = output.shape().size() - 1;
    SizeType batch_size             = A.shape().at(A_batch_dimension);

    for (SizeType i{0}; i < batch_size; i++)
    {
      auto A_slice      = A.Slice(i, A_batch_dimension);
      auto B_slice      = B.Slice(0, B_batch_dimension);
      auto output_slice = output.Slice(i, output_batch_dimension);

      auto A_slice_it      = A_slice.begin();
      auto B_slice_it      = B_slice.begin();
      auto output_slice_it = output_slice.begin();
      while (A_slice_it.is_valid())
      {
        *output_slice_it = *A_slice_it + *B_slice_it;
        ++A_slice_it;
        ++B_slice_it;
        ++output_slice_it;
      }
    }
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape().at(0) == inputs.at(1).get().shape().at(0));
    assert(error_signal.size() == inputs.at(0).get().size());

    SizeType batch_dimension = inputs.front().get().shape().size() - 1;
    return {error_signal, fetch::math::ReduceSum(error_signal, batch_dimension)};
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "BatchwiseAdd";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
