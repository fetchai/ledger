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

#include "math/standard_functions/exp.hpp"
#include "ml/ops/exp.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Exp<TensorType>::Exp(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Exp<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();
  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Exp<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * elementwise exp
 * @param inputs vector containing one tensor which is the input tensor to Exp
 * @return
 */
template <class TensorType>
void Exp<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == this->ComputeOutputShape(inputs));

  fetch::math::Exp((*inputs.at(0)), output);
}

/**
 * elementwise exp gradient is:
 * f'(input0)= e^x * error_signal
 */
template <class TensorType>
std::vector<TensorType> Exp<TensorType>::Backward(VecTensorType const &inputs,
                                                  TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(error_signal.shape() == this->ComputeOutputShape(inputs));

  TensorType ret_error_signal(inputs.at(0)->shape());
  fetch::math::Exp((*inputs.at(0)), ret_error_signal);
  fetch::math::Multiply(error_signal, ret_error_signal, ret_error_signal);

  return {ret_error_signal};
}

template <class TensorType>
std::vector<math::SizeType> Exp<TensorType>::ComputeOutputShape(VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Exp<math::Tensor<int8_t>>;
template class Exp<math::Tensor<int16_t>>;
template class Exp<math::Tensor<int32_t>>;
template class Exp<math::Tensor<int64_t>>;
template class Exp<math::Tensor<uint8_t>>;
template class Exp<math::Tensor<uint16_t>>;
template class Exp<math::Tensor<uint32_t>>;
template class Exp<math::Tensor<uint64_t>>;
template class Exp<math::Tensor<float>>;
template class Exp<math::Tensor<double>>;
template class Exp<math::Tensor<fixed_point::fp32_t>>;
template class Exp<math::Tensor<fixed_point::fp64_t>>;
template class Exp<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
