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
class Flatten : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = SaveableParams;

  Flatten() = default;

  explicit Flatten(SPType const &sp)
    : Ops<T>(sp)
  {}

  virtual ~Flatten() = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams()
  {
    SPType sp{};
    sp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<SPType>(sp);
  }

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 1);
    assert(output.shape() == ComputeOutputShape(inputs));
    input_shape_ = inputs.front().get().shape();

    FlattenImpl(inputs.front().get(), output);
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    FETCH_UNUSED(inputs);
    assert(inputs.size() == 1);
    ArrayType ret(input_shape_);
    FlattenImpl(error_signal, ret);

    return {ret};
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    SizeType batch_size =
        inputs.at(0).get().shape().at(inputs.at(0).get().shape().size() - SizeType{1});
    SizeType data_size = 1;
    for (SizeType i{0}; i < inputs.at(0).get().shape().size() - SizeType{1}; i++)
    {
      data_size *= inputs.at(0).get().shape().at(i);
    }

    return {data_size, batch_size};
  }

  static constexpr char const *DESCRIPTOR = "Flatten";

private:
  std::vector<std::uint64_t> input_shape_;

  void FlattenImpl(ArrayType const &A, ArrayType &ret)
  {
    // Test if batch dimensions are equal
    assert(ret.shape().at(ret.shape().size() - 1) == A.shape().at(A.shape().size() - 1));

    SizeType input_batch_dimension  = A.shape().size() - 1;
    SizeType output_batch_dimension = ret.shape().size() - 1;
    SizeType batch_size             = ret.shape().at(output_batch_dimension);

    for (SizeType i{0}; i < batch_size; i++)
    {
      auto input_slice  = A.Slice(i, input_batch_dimension);
      auto output_slice = ret.Slice(i, output_batch_dimension);
      output_slice.Assign(input_slice);
    }
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
