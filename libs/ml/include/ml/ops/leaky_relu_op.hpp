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

#include "core/assert.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/ml/activation_functions/leaky_relu.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LeakyReluOp : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  LeakyReluOp()          = default;
  virtual ~LeakyReluOp() = default;

  std::shared_ptr<SaveableParams<ArrayType>> GetOpSaveableParams()
  {
    SaveableParams<ArrayType> sp{};
    sp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<SaveableParams<ArrayType>>(sp);
  }

  // LeakyRelu(x,alpha)=max(0,x)+alpha*min(0,x)
  void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape() == output.shape());
    assert(inputs.at(1).get().shape().at(inputs.at(1).get().shape().size() - 1) == 1);

    fetch::math::LeakyRelu(inputs.at(0).get(), inputs.at(1).get(), output);
  }

  // Gradient of input.at(0)=x is:
  //    x>=0 f'(x)=1, x<0 f'(x)=alpha
  // Gradient of input.at(1)=alpha is:
  //    f'(alpha)=-Relu(-x)=min(0,x); x>=0 f'(alpha)=0, x<0 f'(alpha)=x
  std::vector<ArrayType> Backward(VecTensorType const &inputs, ArrayType const &error_signal)
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().size() == error_signal.size());

    // Test if batch dimension for alpha is 1
    assert(inputs.at(1).get().shape().at(inputs.at(1).get().shape().size() - 1) == 1);

    ArrayType return_signal_1{inputs.at(0).get().shape()};

    SizeType a_size{1};
    for (SizeType i{0}; i < inputs.at(0).get().shape().size() - 1; i++)
    {
      a_size *= inputs.at(0).get().shape().at(i);
    }
    ArrayType return_signal_2({a_size, 1});

    SizeType t_batch_dimension = inputs.at(0).get().shape().size() - 1;
    SizeType batch_size        = inputs.at(0).get().shape().at(t_batch_dimension);

    for (SizeType i{0}; i < batch_size; i++)
    {

      // Slice along batch dimension
      auto input1_slice = inputs.at(0).get().Slice(i, t_batch_dimension);
      auto rs1_slice    = return_signal_1.Slice(i, t_batch_dimension);
      auto error_slice  = error_signal.Slice(i, 1);

      auto rs1_it    = rs1_slice.begin();
      auto rs2_it    = return_signal_2.begin();
      auto input1_it = input1_slice.begin();
      auto input2_it = inputs.at(1).get().begin();
      auto error_it  = error_slice.begin();

      while (input1_it.is_valid())
      {
        if (*input1_it >= static_cast<DataType>(0))
        {
          *rs1_it = static_cast<DataType>(1) * (*error_it);
        }
        else
        {
          *rs1_it = (*input2_it) * (*error_it);
          *rs2_it += (*input1_it) * (*error_it);
        }
        ++rs1_it;
        ++rs2_it;
        ++input1_it;
        ++input2_it;
        ++error_it;
      }
    }

    return {return_signal_1, return_signal_2};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "LeakyReluOp";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
