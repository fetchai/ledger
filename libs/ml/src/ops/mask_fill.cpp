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

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

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
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

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
    const std::vector<math::SizeVector> &inputs) const
{
  return inputs.at(1);
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> MaskFill<TensorType>::ChargeForward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_input_shapes_.empty());

  OperationsCount op_cnt = fetch::ml::charge_estimation::ops::MASK_FILL_PER_ELEMENT *
                           TensorType::SizeFromShape(input_shapes[0]);

  auto output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(op_cnt, output_shape);
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> MaskFill<TensorType>::ChargeBackward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_output_shape_.empty());

  OperationsCount cost = fetch::ml::charge_estimation::ops::LOW_MULTIPLICATION_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_output_shape_});
  math::SizeVector output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(cost * output_shape.back(), output_shape);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class MaskFill<math::Tensor<int8_t>>;
template class MaskFill<math::Tensor<int16_t>>;
template class MaskFill<math::Tensor<int32_t>>;
template class MaskFill<math::Tensor<int64_t>>;
template class MaskFill<math::Tensor<float>>;
template class MaskFill<math::Tensor<double>>;
template class MaskFill<math::Tensor<fixed_point::fp32_t>>;
template class MaskFill<math::Tensor<fixed_point::fp64_t>>;
template class MaskFill<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
