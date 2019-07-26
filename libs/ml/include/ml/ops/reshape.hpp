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
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Reshape : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = ReshapeSaveableParams;

  explicit Reshape(std::vector<SizeType> new_shape)
    : new_shape_(new_shape)
  {}

  explicit Reshape(SPType const &sp)
  {
    new_shape_ = sp.new_shape;  // todo: tests for this
  }

  ~Reshape() = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.DESCRIPTOR = DESCRIPTOR;
    sp.new_shape  = new_shape_;
    return std::make_shared<SPType>(sp);
  }

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == ComputeOutputShape(inputs));
    assert(inputs.front()->size() == output.size());

    output.Assign((*inputs.front()));
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    ArrayType ret(inputs.front()->shape());
    ret.Assign(error_signal);
    return {ret};
  }

  // Output shape
  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    std::vector<SizeType> output_size;
    for (SizeType i{0}; i < new_shape_.size(); i++)
    {
      if (new_shape_.at(i) < inputs.front()->shape().size())
      {
        output_size.push_back(inputs.front()->shape().at(new_shape_.at(i)));
      }
      else
      {
        output_size.push_back(1);
      }
    }

    return output_size;
  }

  static constexpr char const *DESCRIPTOR = "Reshape";

private:
  std::vector<SizeType> new_shape_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
