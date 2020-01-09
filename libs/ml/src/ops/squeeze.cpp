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

#include "ml/ops/squeeze.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Squeeze<TensorType>::Squeeze(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Squeeze<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();
  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Squeeze<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * Squeeze removes the first size 1 dimension encountered starting at the trailing edge.
 * If no dimensions are size 1, it will throw.
 * @param inputs vector containing one tensor which is the input tensor to Squeeze
 * @return
 */
template <typename TensorType>
void Squeeze<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == this->ComputeOutputShape(inputs));

  output.Copy((*inputs.at(0)));
  output.Squeeze();
}

/**
 * Just re-assign error to different shaped array:
 * f'(input0)= error_signal
 */
template <typename TensorType>
std::vector<TensorType> Squeeze<TensorType>::Backward(VecTensorType const &inputs,
                                                      TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(error_signal.shape() == this->ComputeOutputShape(inputs));

  TensorType ret_error_signal(inputs.at(0)->shape());
  ret_error_signal.Assign(error_signal);

  return {ret_error_signal};
}

template <typename TensorType>
std::vector<math::SizeType> Squeeze<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{

  auto shape = inputs.front()->shape();

  bool     not_found = true;
  SizeType cur_dim   = shape.size() - 1;
  while (not_found)
  {
    if (shape.at(cur_dim) == static_cast<SizeType>(1))
    {
      shape.erase(shape.begin() + static_cast<int32_t>(cur_dim));
      not_found = false;
    }
    else
    {
      if (cur_dim == 0)
      {
        throw math::exceptions::InvalidReshape("cannot squeeze tensor, no dimensions of size 1");
      }
      --cur_dim;
    }
  }
  return shape;
}
///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Squeeze<math::Tensor<int8_t>>;
template class Squeeze<math::Tensor<int16_t>>;
template class Squeeze<math::Tensor<int32_t>>;
template class Squeeze<math::Tensor<int64_t>>;
template class Squeeze<math::Tensor<uint8_t>>;
template class Squeeze<math::Tensor<uint16_t>>;
template class Squeeze<math::Tensor<uint32_t>>;
template class Squeeze<math::Tensor<uint64_t>>;
template class Squeeze<math::Tensor<float>>;
template class Squeeze<math::Tensor<double>>;
template class Squeeze<math::Tensor<fixed_point::fp32_t>>;
template class Squeeze<math::Tensor<fixed_point::fp64_t>>;
template class Squeeze<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
