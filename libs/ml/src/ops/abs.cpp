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

#include "math/standard_functions/abs.hpp"
#include "ml/ops/abs.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Abs<TensorType>::Abs(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Abs<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Abs<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * elementwise absolute value
 * @param inputs - one input for elementwise abs
 * @return
 */
template <typename TensorType>
void Abs<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(inputs.at(0)->shape() == output.shape());
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  fetch::math::Abs((*inputs.at(0)), output);
}

/**
 * elementwise absolute value gradient is:
 * f'(input0)=sign(input0)*error_signal
 */
template <typename TensorType>
std::vector<TensorType> Abs<TensorType>::Backward(VecTensorType const &inputs,
                                                  TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(error_signal.size() == inputs.at(0)->size());

  TensorType return_signal(inputs.at(0)->shape());

  auto a_it   = inputs.at(0)->cbegin();
  auto err_it = error_signal.cbegin();
  auto r_it   = return_signal.begin();
  while (a_it.is_valid())
  {
    if (*a_it > 0)
    {
      *r_it = static_cast<DataType>(*err_it);
    }
    else
    {
      *r_it = static_cast<DataType>(-*err_it);
    }

    ++a_it;
    ++err_it;
    ++r_it;
  }

  return {return_signal};
}

template <typename TensorType>
std::vector<math::SizeType> Abs<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  return inputs.front();
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> Abs<TensorType>::ChargeForward(
    std::vector<math::SizeVector> const &input_shapes)
{
  OperationsCount op_cnt = fetch::ml::charge_estimation::ops::ABS_PER_ELEMENT *
                           TensorType::SizeFromShape(input_shapes[0]);
  auto output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(op_cnt, output_shape);
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> Abs<TensorType>::ChargeBackward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_output_shape_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::ASSIGN_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_output_shape_});

  math::SizeVector output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(cost * output_shape.back(), output_shape);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Abs<math::Tensor<int8_t>>;
template class Abs<math::Tensor<int16_t>>;
template class Abs<math::Tensor<int32_t>>;
template class Abs<math::Tensor<int64_t>>;
template class Abs<math::Tensor<float>>;
template class Abs<math::Tensor<double>>;
template class Abs<math::Tensor<fixed_point::fp32_t>>;
template class Abs<math::Tensor<fixed_point::fp64_t>>;
template class Abs<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
