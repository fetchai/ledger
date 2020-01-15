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
#include "ml/ops/mask_fill.hpp"
#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> MaskFill<TensorType>::GetOpSaveableParams()
{
  auto sp        = std::make_shared<SPType>();
  sp->fill_value = fill_value_;
  return sp;
}

template <typename TensorType>
MaskFill<TensorType>::MaskFill(MaskFill::DataType fill_value)
  : fill_value_(fill_value)
{}

template <typename TensorType>
MaskFill<TensorType>::MaskFill(const MaskFill::SPType &sp)
  : Ops<TensorType>(sp)
{
  fill_value_ = sp.fill_value;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MaskFill<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}
/**
 * based on boolean condition mask, decide if we need to fill the element with fill_value.
 * @param inputs - two inputs, first is mask, second is the array to be masked
 * array
 * @return
 */
template <typename TensorType>
void MaskFill<TensorType>::Forward(const MaskFill::VecTensorType &inputs,
                                   MaskFill::TensorType &         output)
{
  assert(inputs.size() == 2);
  assert(output.shape() == this->ComputeOutputShape(inputs));

  fetch::math::Multiply(*(inputs.at(0)), *(inputs.at(1)), output);
  TensorType inv_mask = fetch::math::Subtract(DataType{1}, *(inputs.at(0)));
  fetch::math::Multiply(inv_mask, fill_value_, inv_mask);
  fetch::math::Add(output, inv_mask, output);
}

/**
 * elementwise gradient for second input (the then input) is:
 * error' = mask * error_signal
 */

template <typename TensorType>
std::vector<TensorType> MaskFill<TensorType>::Backward(const VecTensorType &inputs,
                                                       const TensorType &   error_signal)
{
  assert(inputs.size() == 2);
  assert(error_signal.size() == inputs.at(1)->size());

  TensorType return_signal(inputs.at(1)->shape());
  TensorType mask_return_signal(inputs.at(0)->shape());

  fetch::math::Multiply(*(inputs.front()), error_signal, return_signal);

  // be adivsed, it is not reasonable to return gradient for mask, so the mask gradient is set to
  // zero here
  return {mask_return_signal, return_signal};
}

template <typename TensorType>
std::vector<fetch::math::SizeType> MaskFill<TensorType>::ComputeOutputShape(
    const VecTensorType &inputs) const
{
  return inputs.at(1)->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class MaskFill<math::Tensor<int8_t>>;
template class MaskFill<math::Tensor<int16_t>>;
template class MaskFill<math::Tensor<int32_t>>;
template class MaskFill<math::Tensor<int64_t>>;
template class MaskFill<math::Tensor<uint8_t>>;
template class MaskFill<math::Tensor<uint16_t>>;
template class MaskFill<math::Tensor<uint32_t>>;
template class MaskFill<math::Tensor<uint64_t>>;
template class MaskFill<math::Tensor<float>>;
template class MaskFill<math::Tensor<double>>;
template class MaskFill<math::Tensor<fixed_point::fp32_t>>;
template class MaskFill<math::Tensor<fixed_point::fp64_t>>;
template class MaskFill<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
