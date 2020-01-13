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

#include "math/activation_functions/sigmoid.hpp"
#include "math/fundamental_operators.hpp"
#include "math/standard_functions/clamp.hpp"
#include "ml/ops/activations/sigmoid.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Sigmoid<TensorType>::Sigmoid(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Sigmoid<TensorType>::GetOpSaveableParams()
{
  return std::make_shared<SPType>();
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Sigmoid<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void Sigmoid<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == this->ComputeOutputShape(inputs));
  fetch::math::Sigmoid(*(inputs.front()), output);

  // ensures numerical stability
  fetch::math::Clamp(epsilon_, static_cast<DataType>(DataType{1} - epsilon_), output);
}

template <typename TensorType>
std::vector<TensorType> Sigmoid<TensorType>::Backward(VecTensorType const &inputs,
                                                      TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.front()->shape() == error_signal.shape());
  TensorType return_signal{error_signal.shape()};
  TensorType t{inputs.front()->shape()};

  // gradient of sigmoid function is s(x)(1 - s(x))
  Forward(inputs, t);
  fetch::math::Subtract(DataType{1}, t, return_signal);
  fetch::math::Multiply(t, return_signal, return_signal);

  // multiply by error_signal (chain rule)
  fetch::math::Multiply(error_signal, return_signal, return_signal);

  return {return_signal};
}

template <typename TensorType>
std::vector<math::SizeType> Sigmoid<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Sigmoid<math::Tensor<int8_t>>;
template class Sigmoid<math::Tensor<int16_t>>;
template class Sigmoid<math::Tensor<int32_t>>;
template class Sigmoid<math::Tensor<int64_t>>;
template class Sigmoid<math::Tensor<uint8_t>>;
template class Sigmoid<math::Tensor<uint16_t>>;
template class Sigmoid<math::Tensor<uint32_t>>;
template class Sigmoid<math::Tensor<uint64_t>>;
template class Sigmoid<math::Tensor<float>>;
template class Sigmoid<math::Tensor<double>>;
template class Sigmoid<math::Tensor<fixed_point::fp32_t>>;
template class Sigmoid<math::Tensor<fixed_point::fp64_t>>;
template class Sigmoid<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
