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

#include "math/activation_functions/leaky_relu.hpp"
#include "math/fundamental_operators.hpp"
#include "ml/ops/prelu_op.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
PReluOp<TensorType>::PReluOp(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> PReluOp<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();
  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> PReluOp<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

// PRelu(x,alpha)=max(x,x*alpha)
template <typename TensorType>
void PReluOp<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->shape() == output.shape());
  assert(inputs.at(1)->shape().at(inputs.at(1)->shape().size() - 1) == 1);

  fetch::math::LeakyRelu((*inputs.at(0)), (*inputs.at(1)), output);
}

// Gradient of input.at(0)=x is:
//    x>=0 f'(x)=1, x<0 f'(x)=alpha
// Gradient of input.at(1)=alpha is:
//    f'(alpha)=-Relu(-x)=min(0,x); x>=0 f'(alpha)=0, x<0 f'(alpha)=x
template <typename TensorType>
std::vector<TensorType> PReluOp<TensorType>::Backward(VecTensorType const &inputs,
                                                      TensorType const &   error_signal)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == error_signal.size());

  // Test if batch dimension for alpha is 1
  assert(inputs.at(1)->shape().at(inputs.at(1)->shape().size() - 1) == 1);

  TensorType return_signal_1{inputs.at(0)->shape()};

  SizeType a_size{1};
  for (SizeType i{0}; i < inputs.at(0)->shape().size() - 1; i++)
  {
    a_size *= inputs.at(0)->shape().at(i);
  }
  TensorType return_signal_2({a_size, 1});

  SizeType t_batch_dimension = inputs.at(0)->shape().size() - 1;
  SizeType batch_size        = inputs.at(0)->shape().at(t_batch_dimension);

  for (SizeType i{0}; i < batch_size; i++)
  {

    // View along batch dimension
    auto input1_view = inputs.at(0)->View(i);
    auto rs1_view    = return_signal_1.View(i);
    auto error_view  = error_signal.View(i);

    auto rs1_it    = rs1_view.begin();
    auto rs2_it    = return_signal_2.begin();
    auto input1_it = input1_view.begin();
    auto input2_it = inputs.at(1)->begin();
    auto error_it  = error_view.begin();

    while (input1_it.is_valid())
    {
      if (*input1_it >= DataType{0})
      {
        *rs1_it = *error_it;
      }
      else
      {
        *rs1_it = static_cast<DataType>((*input2_it) * (*error_it));
        *rs2_it = static_cast<DataType>(*rs2_it + ((*input1_it) * (*error_it)));
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

template <typename TensorType>
std::vector<math::SizeType> PReluOp<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class PReluOp<math::Tensor<int8_t>>;
template class PReluOp<math::Tensor<int16_t>>;
template class PReluOp<math::Tensor<int32_t>>;
template class PReluOp<math::Tensor<int64_t>>;
template class PReluOp<math::Tensor<uint8_t>>;
template class PReluOp<math::Tensor<uint16_t>>;
template class PReluOp<math::Tensor<uint32_t>>;
template class PReluOp<math::Tensor<uint64_t>>;
template class PReluOp<math::Tensor<float>>;
template class PReluOp<math::Tensor<double>>;
template class PReluOp<math::Tensor<fixed_point::fp32_t>>;
template class PReluOp<math::Tensor<fixed_point::fp64_t>>;
template class PReluOp<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
