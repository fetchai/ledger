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

//#include "core/assert.hpp"
#include "math/activation_functions/gelu.hpp"
#include "math/trigonometry.hpp"
#include "ml/ops/activations/gelu.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Gelu<TensorType>::Gelu(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Gelu<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();
  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Gelu<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

// 0.5x(1+tanh(0.797885x+0.035677x^3))
template <typename TensorType>
void Gelu<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == this->ComputeOutputShape(inputs));
  fetch::math::Gelu(*(inputs.front()), output);
}

/**
 * Gradients for backprop with Gelu are as follows:
 * a = 0.797885, b = 0.035677
 * 0.5 * (1 + tanh(ax + bx^3) + x * sech^2(ax + bx^3)(a + 3bx^2))
 * @param inputs
 * @param error_signal
 * @return
 */
template <typename TensorType>
std::vector<TensorType> Gelu<TensorType>::Backward(VecTensorType const &inputs,
                                                   TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.at(0)->shape() == error_signal.shape());

  TensorType const &input = *(inputs.front());
  TensorType        intermediate1({input.shape()}), intermediate2({input.shape()}),
      intermediate3({input.shape()});

  DataType const one{1};
  DataType const two{2};
  DataType const three{3};
  DataType const half = fetch::math::Type<DataType>("0.5");
  DataType const a    = fetch::math::Type<DataType>("0.797885");
  DataType const b    = fetch::math::Type<DataType>("0.035677");

  // get ax + bx^3
  fetch::math::Multiply(input, a, intermediate1);
  fetch::math::Pow(input, three, intermediate2);
  fetch::math::Multiply(intermediate2, b, intermediate2);
  fetch::math::Add(intermediate1, intermediate2, intermediate1);

  // get tanh() term
  fetch::math::TanH(intermediate1, intermediate2);

  // get x * sech^2() term
  fetch::math::CosH(intermediate1, intermediate3);
  fetch::math::Pow(intermediate3, two, intermediate3);
  fetch::math::Divide(one, intermediate3, intermediate3);
  fetch::math::Multiply(input, intermediate3, intermediate3);

  // get a + 3bx^2 term
  fetch::math::Pow(input, two, intermediate1);
  fetch::math::Multiply(intermediate1, b, intermediate1);
  fetch::math::Multiply(intermediate1, three, intermediate1);
  fetch::math::Add(intermediate1, a, intermediate1);

  // 1 + tanh(ax + bx^3) + x * sech^2(ax + bx^3)(a + 3bx^2)
  fetch::math::Multiply(intermediate1, intermediate3, intermediate1);
  fetch::math::Add(intermediate1, intermediate2, intermediate1);
  fetch::math::Add(intermediate1, one, intermediate1);

  // final value
  fetch::math::Multiply(intermediate1, half, intermediate1);

  fetch::math::Multiply(intermediate1, error_signal, intermediate1);

  return {intermediate1};
}

template <typename TensorType>
std::vector<math::SizeType> Gelu<TensorType>::ComputeOutputShape(VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Gelu<math::Tensor<int8_t>>;
template class Gelu<math::Tensor<int16_t>>;
template class Gelu<math::Tensor<int32_t>>;
template class Gelu<math::Tensor<int64_t>>;
template class Gelu<math::Tensor<uint8_t>>;
template class Gelu<math::Tensor<uint16_t>>;
template class Gelu<math::Tensor<uint32_t>>;
template class Gelu<math::Tensor<uint64_t>>;
template class Gelu<math::Tensor<float>>;
template class Gelu<math::Tensor<double>>;
template class Gelu<math::Tensor<fixed_point::fp32_t>>;
template class Gelu<math::Tensor<fixed_point::fp64_t>>;
template class Gelu<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
