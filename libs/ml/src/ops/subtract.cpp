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

#include "ml/ops/subtract.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Subtract<TensorType>::Subtract(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Subtract<TensorType>::GetOpSaveableParams()
{
  SPType sp{};
  return std::make_shared<SPType>(sp);
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Subtract<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <class TensorType>
void Subtract<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());
  assert(output.shape() == this->ComputeOutputShape(inputs));

  fetch::math::Subtract((*inputs.at(0)), (*inputs.at(1)), output);
}

template <class TensorType>
std::vector<TensorType> Subtract<TensorType>::Backward(VecTensorType const &inputs,
                                                       TensorType const &   error_signal)
{
  FETCH_UNUSED(inputs);
  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());
  assert(error_signal.size() == inputs.at(1)->size());

  return {error_signal, fetch::math::Multiply(error_signal, DataType{-1})};
}

template <class TensorType>
std::vector<math::SizeType> Subtract<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Subtract<math::Tensor<int8_t>>;
template class Subtract<math::Tensor<int16_t>>;
template class Subtract<math::Tensor<int32_t>>;
template class Subtract<math::Tensor<int64_t>>;
template class Subtract<math::Tensor<float>>;
template class Subtract<math::Tensor<double>>;
template class Subtract<math::Tensor<fixed_point::fp32_t>>;
template class Subtract<math::Tensor<fixed_point::fp64_t>>;
template class Subtract<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
