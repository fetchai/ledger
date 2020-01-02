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
#include "ml/ops/ops.hpp"

#include <cassert>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Reshape : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpReshapeSaveableParams<T>;
  using MyType        = Reshape<TensorType>;

  explicit Reshape(std::vector<SizeType> new_shape)
    : new_shape_(std::move(new_shape))
    , new_size_(fetch::math::Product(new_shape_))
  {
    assert(new_shape_.size() > 1);
  }

  explicit Reshape(SPType const &sp)
    : Ops<T>(sp)
    , new_shape_(sp.new_shape)
    , new_size_(sp.new_size)
  {}

  ~Reshape() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.new_shape = new_shape_;
    sp.new_size  = new_size_;
    return std::make_shared<SPType>(sp);
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);
    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType
    return copyshare;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == ComputeOutputShape(inputs));

    // if batch sizes don't agree - update specified new_shape
    SizeType input_batch_size = inputs.at(0)->shape(inputs.at(0)->shape().size() - 1);
    SizeType new_batch_size   = new_shape_.at(new_shape_.size() - 1);
    if (input_batch_size != new_batch_size)
    {
      new_shape_.at(new_shape_.size() - 1) = input_batch_size;
      new_size_                            = fetch::math::Product(new_shape_);
    }

    // if the shape is exactly the same just copy the data
    if ((*inputs.front()).shape() == new_shape_)
    {
      output.Assign((*inputs.front()));
    }
    // check the reshape sizes match!
    else if ((*inputs.front()).size() != new_size_)
    {
      throw fetch::math::exceptions::WrongShape(
          "new shape has different size from current tensor size");
    }
    else
    {
      output.Reshape(new_shape_);
      output.Assign((*inputs.front()));
    }
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);

    TensorType ret(inputs.at(0)->shape());
    ret.Assign(error_signal);
    return {ret};
  }

  // Output shape
  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    assert(inputs.size() == 1);
    assert(inputs.at(0)->shape().size() > 1);

    // output shape prespecified (except batch dimension)
    std::vector<SizeType> output_shape = new_shape_;

    // overwrite the batch dimension
    output_shape.at(output_shape.size() - 1) =
        inputs.at(0)->shape().at(inputs.at(0)->shape().size() - 1);

    return output_shape;
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_RESHAPE;
  }
  static constexpr char const *DESCRIPTOR = "Reshape";

private:
  std::vector<SizeType> new_shape_;
  SizeType              new_size_{0};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
