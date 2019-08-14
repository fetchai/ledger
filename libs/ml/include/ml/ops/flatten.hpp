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

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Flatten : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = typename TensorType::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpFlattenSaveableParams<T>;

  Flatten() = default;

  explicit Flatten(SPType const &sp)
    : Ops<T>(sp)
  {
    input_shape_ = sp.input_shape;
  }

  ~Flatten() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto ret         = std::make_shared<SPType>();
    ret->input_shape = input_shape_;
    return ret;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == ComputeOutputShape(inputs));
    input_shape_ = inputs.front()->shape();

    assert(output.shape().at(output.shape().size() - 1) ==
           inputs.front()->shape().at(inputs.front()->shape().size() - 1));
    output.Assign(inputs.front()->View());
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.size() == 1);
    TensorType ret(input_shape_);

    assert(ret.shape().at(ret.shape().size() - 1) ==
           error_signal.shape().at(error_signal.shape().size() - 1));
    ret.Assign(error_signal.View());

    return {ret};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    SizeType batch_size = inputs.at(0)->shape().at(inputs.at(0)->shape().size() - SizeType{1});
    SizeType data_size  = 1;
    for (SizeType i{0}; i < inputs.at(0)->shape().size() - SizeType{1}; i++)
    {
      data_size *= inputs.at(0)->shape().at(i);
    }

    return {data_size, batch_size};
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_FLATTEN;
  }
  static constexpr char const *DESCRIPTOR = "Flatten";

private:
  std::vector<SizeType> input_shape_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
