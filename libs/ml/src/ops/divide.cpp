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

#include "math/fundamental_operators.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/ops/divide.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Divide<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();
  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Divide<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * elementwise division
 * @param inputs  left & right inputs to Divide
 * @return
 */
template <class TensorType>
void Divide<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->shape() == output.shape());

  if ((inputs.at(0)->shape() == inputs.at(1)->shape()) || (inputs.at(1)->size() > 1))
  {  // array / array
    fetch::math::Divide(*inputs.at(0), *inputs.at(1), output);
  }
  else
  {  // array / scalar
    fetch::math::Divide(*inputs.at(0), *(inputs.at(1)->cbegin()), output);
  }
}

/**
 * f'(a)=(1/b)*err
 * f'(b)=-(a/(b^2))*err
 */
template <class TensorType>
std::vector<TensorType> Divide<TensorType>::Backward(VecTensorType const &inputs,
                                                     TensorType const &   error_signal)
{
  TensorType return_signal_1(inputs.at(0)->shape());
  TensorType return_signal_2(inputs.at(1)->shape());

  auto a_it   = inputs.at(0)->cbegin();
  auto b_it   = inputs.at(1)->cbegin();
  auto err_it = error_signal.cbegin();
  auto r_1_it = return_signal_1.begin();
  auto r_2_it = return_signal_2.begin();
  if (inputs.at(0)->shape() == inputs.at(1)->shape())
  {  // array / array same shape
    while (a_it.is_valid())
    {
      *r_1_it = static_cast<DataType>((*err_it) / (*b_it));
      *r_2_it = static_cast<DataType>(-((*err_it) * (*a_it)) / ((*b_it) * (*b_it)));

      ++a_it;
      ++b_it;
      ++err_it;
      ++r_1_it;
      ++r_2_it;
    }
  }
  else if (inputs.at(1)->size() == 1)
  {  // array / scalar
    while (a_it.is_valid())
    {
      *r_1_it = static_cast<DataType>((*err_it) / (*b_it));
      *r_2_it += static_cast<DataType>(-((*err_it) * (*a_it)) / ((*b_it) * (*b_it)));

      ++a_it;
      ++err_it;
      ++r_1_it;
    }
  }
  else
  {  // array / array different shape
     // TODO (#1380) Write backpropagation for array array division of different shapes
    throw ml::exceptions::NotImplemented(
        "array array division of different shapes is not yet handled");
  }
  return {return_signal_1, return_signal_2};
}

template <class TensorType>
std::vector<math::SizeType> Divide<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Divide<math::Tensor<int8_t>>;
template class Divide<math::Tensor<int16_t>>;
template class Divide<math::Tensor<int32_t>>;
template class Divide<math::Tensor<int64_t>>;
template class Divide<math::Tensor<uint8_t>>;
template class Divide<math::Tensor<uint16_t>>;
template class Divide<math::Tensor<uint32_t>>;
template class Divide<math::Tensor<uint64_t>>;
template class Divide<math::Tensor<float>>;
template class Divide<math::Tensor<double>>;
template class Divide<math::Tensor<fixed_point::fp32_t>>;
template class Divide<math::Tensor<fixed_point::fp64_t>>;
template class Divide<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
