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
#include "ml/ops/activations/leaky_relu.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
LeakyRelu<TensorType>::LeakyRelu(DataType a)
  : a_(a)
{}

template <typename TensorType>
LeakyRelu<TensorType>::LeakyRelu(SPType const &sp)
  : Ops<TensorType>(sp)
{
  a_ = sp.a;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> LeakyRelu<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();
  sp->a   = a_;

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> LeakyRelu<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void LeakyRelu<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  fetch::math::LeakyRelu((*inputs.front()), a_, output);
}

template <typename TensorType>
std::vector<TensorType> LeakyRelu<TensorType>::Backward(VecTensorType const &inputs,
                                                        TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.front()->shape() == error_signal.shape());
  DataType   zero{0};
  DataType   one{1};
  TensorType ret{error_signal.shape()};
  TensorType t{inputs.front()->shape()};

  // gradient of leaky relu function is a where x<0; and 1.0 where x>=0
  this->Forward(inputs, t);

  auto it  = t.cbegin();
  auto rit = ret.begin();
  while (it.is_valid())
  {
    if (*it >= zero)
    {
      // f'(x)=1 for x>=0
      *rit = one;
    }
    else
    {
      // f'(x)=a for x<0
      *rit = a_;
    }
    ++it;
    ++rit;
  }

  // multiply by error_signal (chain rule)
  fetch::math::Multiply(error_signal, ret, ret);

  return {ret};
}

template <typename TensorType>
std::vector<math::SizeType> LeakyRelu<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  return inputs.front();
}

template <typename TensorType>
OperationsCount LeakyRelu<TensorType>::ChargeForward() const
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::LEAKY_RELU_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  return cost;
}

template <typename TensorType>
OperationsCount LeakyRelu<TensorType>::ChargeBackward() const
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::LEAKY_RELU_BACKWARD_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  return cost;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class LeakyRelu<math::Tensor<int8_t>>;
template class LeakyRelu<math::Tensor<int16_t>>;
template class LeakyRelu<math::Tensor<int32_t>>;
template class LeakyRelu<math::Tensor<int64_t>>;
template class LeakyRelu<math::Tensor<float>>;
template class LeakyRelu<math::Tensor<double>>;
template class LeakyRelu<math::Tensor<fixed_point::fp32_t>>;
template class LeakyRelu<math::Tensor<fixed_point::fp64_t>>;
template class LeakyRelu<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
