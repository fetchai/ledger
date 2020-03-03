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
#include "ml/ops/maximum.hpp"

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <typename T>
std::shared_ptr<OpsSaveableParams> Maximum<T>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Maximum<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * elementwise maximum
 * @param inputs  left & right inputs to get maximum
 * @return
 */
template <typename T>
void Maximum<T>::Forward(const VecTensorType &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  fetch::math::Maximum((*inputs.at(0)), (*inputs.at(1)), output);
}

/**
 * elementwise maximum gradient is:
 * f'(input0)=if(input0>input1)=error_signal
 * f'(input1)=if(input0<=input1)=error_signal
 */
template <typename TensorType>
std::vector<TensorType> Maximum<TensorType>::Backward(const VecTensorType &inputs,
                                                      const TensorType &   error_signal)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());
  assert(error_signal.size() == inputs.at(1)->size());

  TensorType return_signal_1(inputs.at(0)->shape());
  TensorType return_signal_2(inputs.at(1)->shape());

  auto a_it   = inputs.at(0)->cbegin();
  auto b_it   = inputs.at(1)->cbegin();
  auto err_it = error_signal.cbegin();
  auto r_1_it = return_signal_1.begin();
  auto r_2_it = return_signal_2.begin();
  while (a_it.is_valid())
  {
    if ((*a_it) > (*b_it))
    {
      *r_1_it = *err_it;
    }
    else
    {
      *r_2_it = *err_it;
    }

    ++a_it;
    ++b_it;
    ++err_it;
    ++r_1_it;
    ++r_2_it;
  }

  return {return_signal_1, return_signal_2};
}

template <typename T>
std::vector<fetch::math::SizeType> Maximum<T>::ComputeOutputShape(
    const std::vector<math::SizeVector> &inputs) const
{
  return inputs.front();
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> Maximum<TensorType>::ChargeForward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_input_shapes_.empty());

  OperationsCount op_cnt = fetch::ml::charge_estimation::ops::MAX_PER_ELEMENT *
                           TensorType::SizeFromShape(input_shapes[0]);

  auto output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(op_cnt, output_shape);
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> Maximum<TensorType>::ChargeBackward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_input_shapes_.empty());

  OperationsCount cost = fetch::ml::charge_estimation::ops::ASSIGN_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  math::SizeVector output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(cost * output_shape.back(), output_shape);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Maximum<math::Tensor<int8_t>>;
template class Maximum<math::Tensor<int16_t>>;
template class Maximum<math::Tensor<int32_t>>;
template class Maximum<math::Tensor<int64_t>>;
template class Maximum<math::Tensor<float>>;
template class Maximum<math::Tensor<double>>;
template class Maximum<math::Tensor<fixed_point::fp32_t>>;
template class Maximum<math::Tensor<fixed_point::fp64_t>>;
template class Maximum<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
