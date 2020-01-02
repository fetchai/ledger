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

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Squeeze : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpSqueezeSaveableParams<T>;
  using MyType        = Squeeze<TensorType>;

  Squeeze() = default;

  explicit Squeeze(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Squeeze() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp = std::make_shared<SPType>();
    return sp;
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

    return copyshare;
  }

  /**
   * Squeeze removes the first size 1 dimension encountered starting at the trailing edge.
   * If no dimensions are size 1, it will throw.
   * @param inputs vector containing one tensor which is the input tensor to Squeeze
   * @return
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    output.Copy((*inputs.at(0)));
    output.Squeeze();
  }

  /**
   * Just re-assign error to different shaped array:
   * f'(input0)= error_signal
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    TensorType ret_error_signal(inputs.at(0)->shape());
    ret_error_signal.Assign(error_signal);

    return {ret_error_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {

    auto shape = inputs.front()->shape();

    bool     not_found = true;
    SizeType cur_dim   = shape.size() - 1;
    while (not_found)
    {
      if (shape.at(cur_dim) == static_cast<SizeType>(1))
      {
        shape.erase(shape.begin() + static_cast<int32_t>(cur_dim));
        not_found = false;
      }
      else
      {
        if (cur_dim == 0)
        {
          throw math::exceptions::InvalidReshape("cannot squeeze tensor, no dimensions of size 1");
        }
        --cur_dim;
      }
    }
    return shape;
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_SQUEEZE;
  }
  static constexpr char const *DESCRIPTOR = "Squeeze";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
