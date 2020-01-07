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

#include "core/assert.hpp"
#include "math/activation_functions/relu.hpp"
#include "ml/ops/activations/relu.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Relu<TensorType>::Relu(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Relu<TensorType>::GetOpSaveableParams()
{
  return std::make_shared<SPType>();
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Relu<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

// f(x)=max(0,x);
template <typename TensorType>
void Relu<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == this->ComputeOutputShape(inputs));
  fetch::math::Relu((*inputs.front()), output);
}

/**
 * Gradients for backprop with Relu are as follows:
 * x>0 f'(x)=1, x<=0 f'(x)=0
 * therefore we should return error_signal but zeroed out at the relevant places
 * @param inputs
 * @param error_signal
 * @return
 */
template <typename TensorType>
std::vector<TensorType> Relu<TensorType>::Backward(VecTensorType const &inputs,
                                                  TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.at(0)->shape() == error_signal.shape());

  TensorType const &input = (*inputs.front());
  TensorType        return_signal{error_signal.shape()};

  auto it1    = input.begin();
  auto it2    = return_signal.begin();
  auto err_it = error_signal.cbegin();

  while (it1.is_valid())
  {
    if (*it1 <= DataType{0})
    {
      *it2 = DataType{0};
    }
    else
    {
      *it2 = *err_it;
    }
    ++it1;
    ++it2;
    ++err_it;
  }

  return {return_signal};
}

template <typename TensorType>
std::vector<math::SizeType> Relu<TensorType>::ComputeOutputShape(VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Relu<math::Tensor<int8_t>>;
template class Relu<math::Tensor<int16_t>>;
template class Relu<math::Tensor<int32_t>>;
template class Relu<math::Tensor<int64_t>>;
template class Relu<math::Tensor<uint8_t>>;
template class Relu<math::Tensor<uint16_t>>;
template class Relu<math::Tensor<uint32_t>>;
template class Relu<math::Tensor<uint64_t>>;
template class Relu<math::Tensor<float>>;
template class Relu<math::Tensor<double>>;
template class Relu<math::Tensor<fixed_point::fp32_t>>;
template class Relu<math::Tensor<fixed_point::fp64_t>>;
template class Relu<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
