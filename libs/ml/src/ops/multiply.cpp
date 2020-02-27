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
#include "ml/ops/multiply.hpp"

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <typename T>
std::shared_ptr<OpsSaveableParams> Multiply<T>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Multiply<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * elementwise multiplication
 * for inputs to the multiply layer, if broadcasting is required, make sure the first input is the
 * one with the complete shape
 *
 * @param inputs  left & right inputs to multiply
 * @return
 */
template <typename T>
void Multiply<T>::Forward(const VecTensorType &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->shape().size() <=
         3);  // we do not support input of more than 3D (including batch dims)
  assert(inputs.at(0)->shape().size() ==
         inputs.at(1)->shape().size());  // check if addition is broadcastable
  assert(output.shape() == inputs.front()->shape());

  fetch::math::Multiply((*inputs.at(0)), (*inputs.at(1)), output);
}

/**
 * elementwise multiplication gradient is:
 * f'(input0)=input1*error_signal
 * f'(input1)=input0*error_signal
 */
template <typename TensorType>
std::vector<TensorType> Multiply<TensorType>::Backward(const VecTensorType &inputs,
                                                       const TensorType &   error_signal)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->shape().size() <=
         3);  // we do not support input of more than 3D (including batch dims)
  assert(inputs.at(0)->shape().size() ==
         inputs.at(1)->shape().size());  // check if addition is broadcastable
  assert(error_signal.shape() == inputs.front()->shape());

  TensorType error_signal_1(error_signal.shape());
  TensorType error_signal_2(error_signal.shape());
  fetch::math::Multiply(error_signal, (*inputs.at(1)), error_signal_1);
  fetch::math::Multiply(error_signal, (*inputs.at(0)), error_signal_2);

  if (inputs.at(0)->shape() == inputs.at(1)->shape())
  {
    return {error_signal_1, error_signal_2};
  }
  if (inputs.at(1)->size() == 1)
  {
    // if second input is a scalar
    auto second_error_signal = TensorType(inputs.at(1)->shape());
    fetch::math::Sum(error_signal_2, *second_error_signal.begin());
    return {error_signal_1, second_error_signal};
  }

  // since the shape is not compatible, then the second input must have size 1 in batch dims
  SizeType batch_dimension = inputs.at(0)->shape().size() - 1;
  assert(inputs.at(1)->shape().at(batch_dimension) == 1);
  if (inputs.at(1)->shape().size() == 2)
  {
    // NB * N1 case
    return {error_signal_1, fetch::math::ReduceSum(error_signal_2, batch_dimension)};
  }

  // in the case where we have three dims
  // We only support backward broadcast through shape (N, 1, 1)
  assert(inputs.at(1)->shape(1) == 1);

  TensorType error_sum({inputs.at(1)->shape(0), 1});
  for (SizeType batch = 0; batch < error_signal.shape(batch_dimension); batch++)
  {
    error_sum += fetch::math::ReduceSum(error_signal_2.View(batch).Copy(), SizeType(1));
  }
  error_sum.Reshape(inputs.at(1)->shape());
  return {error_signal_1, error_sum};
}

template <typename T>
std::vector<fetch::math::SizeType> Multiply<T>::ComputeOutputShape(
    const std::vector<math::SizeVector> &inputs) const
{
  return inputs.front();
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> Multiply<TensorType>::ChargeForward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_output_shape_.empty());

  OperationsCount cost = fetch::ml::charge_estimation::ops::OP_OVERHEAD;

  auto output_shape = ComputeOutputShape(input_shapes);
  auto n_elements   = TensorType::SizeFromShape(output_shape);
  auto padded_size  = TensorType::PaddedSizeFromShape(output_shape);

  if ((padded_size / 32) < fetch::ml::charge_estimation::ops::PIECEWISE_LOWER_THRESHOLD)
  {
    // Addition cost
    cost += fetch::ml::charge_estimation::ops::LOW_MULTIPLICATION_PER_ELEMENT * n_elements;

    // Iteration over 3 tensors (input1, input2, ret)
    OperationsCount iteration_ops = TensorType::ChargeIterate(output_shape);
    cost += iteration_ops * 3;
  }
  else if ((padded_size / 32) < fetch::ml::charge_estimation::ops::PIECEWISE_HARD_CAP)
  {
    // Addition cost
    cost += fetch::ml::charge_estimation::ops::HIGH_MULTIPLICATION_PER_ELEMENT * n_elements;

    // Iteration over 3 tensors (input1, input2, ret)
    OperationsCount iteration_ops = TensorType::ChargeIterate(output_shape);
    cost += iteration_ops * 3;
  }
  else
  {
    cost = math::numeric_max<OperationsCount>();
  }

  return std::make_pair(cost, output_shape);
}

template <typename TensorType>
OperationsCount Multiply<TensorType>::ChargeBackward() const
{
  assert(!this->batch_output_shape_.empty());

  OperationsCount cost = fetch::ml::charge_estimation::ops::MULTIPLICATION_BACKWARD_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_output_shape_});
  return cost;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Multiply<math::Tensor<int8_t>>;
template class Multiply<math::Tensor<int16_t>>;
template class Multiply<math::Tensor<int32_t>>;
template class Multiply<math::Tensor<int64_t>>;
template class Multiply<math::Tensor<float>>;
template class Multiply<math::Tensor<double>>;
template class Multiply<math::Tensor<fixed_point::fp32_t>>;
template class Multiply<math::Tensor<fixed_point::fp64_t>>;
template class Multiply<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
