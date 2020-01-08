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

#include "math/activation_functions/elu.hpp"
#include "math/fundamental_operators.hpp"
#include "ml/ops/activations/elu.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Elu<TensorType>::Elu(DataType a)
  : a_(a)
{}

template <typename TensorType>
Elu<TensorType>::Elu(SPType const &sp)
  : Ops<TensorType>(sp)
{
  a_ = sp.a;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Elu<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();
  sp->a   = a_;
  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Elu<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void Elu<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  fetch::math::Elu((*inputs.front()), a_, output);
}

template <typename TensorType>
std::vector<TensorType> Elu<TensorType>::Backward(VecTensorType const &inputs,
                                                  TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.front()->shape() == error_signal.shape());
  TensorType ret{error_signal.shape()};

  auto const zero = DataType{0};
  auto const one  = DataType{1};

  // gradient of elu function is a*e^x where x<0; and 1.0 where x>=0
  auto it  = inputs.front()->cbegin();
  auto rit = ret.begin();
  while (it.is_valid())
  {
    if (*it >= zero)
    {
      // f(x)=x for x>=0
      *rit = one;
    }
    else
    {
      // f(x)=a*e^x
      fetch::math::Exp(*it, *rit);
      fetch::math::Multiply(a_, *rit, *rit);
    }
    ++it;
    ++rit;
  }

  // multiply by error_signal (chain rule)
  fetch::math::Multiply(error_signal, ret, ret);

  return {ret};
}

template <typename TensorType>
std::vector<math::SizeType> Elu<TensorType>::ComputeOutputShape(VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Elu<math::Tensor<int8_t>>;
template class Elu<math::Tensor<int16_t>>;
template class Elu<math::Tensor<int32_t>>;
template class Elu<math::Tensor<int64_t>>;
template class Elu<math::Tensor<uint8_t>>;
template class Elu<math::Tensor<uint16_t>>;
template class Elu<math::Tensor<uint32_t>>;
template class Elu<math::Tensor<uint64_t>>;
template class Elu<math::Tensor<float>>;
template class Elu<math::Tensor<double>>;
template class Elu<math::Tensor<fixed_point::fp32_t>>;
template class Elu<math::Tensor<fixed_point::fp64_t>>;
template class Elu<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
