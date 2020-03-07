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
#include "math/matrix_operations.hpp"
#include "math/trigonometry.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/saveparams/saveable_params.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/math/max.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
TanH<TensorType>::TanH(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> TanH<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> TanH<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <class TensorType>
void TanH<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));
  fetch::math::TanH(*(inputs.front()), output);
  // ensures numerical stability
  for (auto &val : output)
  {
    // Minimum value of tanh is restricted to -1+epsilon
    val = fetch::vectorise::Max(val, fetch::math::Add(DataType(-1), epsilon_));

    // Maximum value of tanh is restricted to 1-epsilon
    val = fetch::vectorise::Min(val, fetch::math::Subtract(DataType{1}, epsilon_));
  }
}

template <class TensorType>
std::vector<TensorType> TanH<TensorType>::Backward(VecTensorType const &inputs,
                                                   TensorType const &   error_signal)
{
  assert(inputs.size() == 1);

  assert(inputs.front()->shape() == error_signal.shape());

  TensorType return_signal = error_signal.Copy();

  TensorType t(ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));
  Forward(inputs, t);

  // gradient of tanh: 1 - tanh(x)^2
  fetch::math::Multiply(t, t, t);
  fetch::math::Subtract(DataType{1}, t, t);

  // apply chain rule
  fetch::math::Multiply(error_signal, t, return_signal);

  return {return_signal};
}

template <class TensorType>
std::vector<math::SizeType> TanH<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  return inputs.front();
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> TanH<TensorType>::ChargeForward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_input_shapes_.empty());

  OperationsCount op_cnt = fetch::ml::charge_estimation::ops::TANH_PER_ELEMENT *
                           TensorType::SizeFromShape(input_shapes[0]);

  auto output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(op_cnt, output_shape);
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> TanH<TensorType>::ChargeBackward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_output_shape_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::TANH_BACKWARD_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_output_shape_});
  math::SizeVector output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(cost * output_shape.back(), output_shape);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class TanH<math::Tensor<int8_t>>;
template class TanH<math::Tensor<int16_t>>;
template class TanH<math::Tensor<int32_t>>;
template class TanH<math::Tensor<int64_t>>;
template class TanH<math::Tensor<float>>;
template class TanH<math::Tensor<double>>;
template class TanH<math::Tensor<fixed_point::fp32_t>>;
template class TanH<math::Tensor<fixed_point::fp64_t>>;
template class TanH<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
