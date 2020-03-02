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

#include "math/activation_functions/softmax.hpp"
#include "math/standard_functions/clamp.hpp"
#include "ml/ops/activations/softmax.hpp"

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Softmax<TensorType>::Softmax(SizeType axis)
  : axis_(axis)
{}

template <typename TensorType>
Softmax<TensorType>::Softmax(std::vector<SizeType> axes)
  : axes_(std::move(axes))
{}

template <typename TensorType>
Softmax<TensorType>::Softmax(SPType const &sp)
  : Ops<TensorType>(sp)
{
  axis_ = sp.axis;
  axes_ = sp.axes;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Softmax<TensorType>::GetOpSaveableParams()
{
  auto sp  = std::make_shared<SPType>();
  sp->axis = axis_;
  sp->axes = axes_;

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Softmax<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void Softmax<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));
  assert(inputs.size() == 1);

  if (axes_.empty())
  {
    fetch::math::Softmax((*inputs.at(0)), output, axis_);
  }
  else
  {
    fetch::math::Softmax((*inputs.at(0)), output, axes_);
  }

  // Clamping ensures numerical stability
  math::Clamp(epsilon_, one_minus_epsilon_, output);
}

template <typename TensorType>
std::vector<TensorType> Softmax<TensorType>::Backward(VecTensorType const &inputs,
                                                      TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.front()->shape() == error_signal.shape());

  TensorType return_signal = error_signal.Copy();
  TensorType t(error_signal.shape());
  this->Forward(inputs, t);

  fetch::math::Multiply(return_signal, t, return_signal);

  // 1D softmax with 1 batch dimension
  if (inputs.front()->shape().size() == 1)
  {
    typename TensorType::Type sum = fetch::math::Sum(return_signal);
    fetch::math::Multiply(t, sum, t);
  }
  // N-D softmax
  else
  {
    if (axes_.empty())
    {
      TensorType sum = ReduceSum(return_signal, axis_);
      fetch::math::Multiply(t, sum, t);
    }
    else
    {
      TensorType sum = ReduceSum(return_signal, axes_);
      fetch::math::Multiply(t, sum, t);
    }
  }

  fetch::math::Subtract(return_signal, t, return_signal);

  return {return_signal};
}

template <typename TensorType>
std::vector<math::SizeType> Softmax<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  return inputs.front();
}

template <typename TensorType>
OperationsCount Softmax<TensorType>::ChargeForward() const
{
  assert(!this->batch_output_shape_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::SOFTMAX_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  return cost;
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> Softmax<TensorType>::ChargeBackward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::SOFTMAX_BACKWARD_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  math::SizeVector output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(cost * output_shape.back(), output_shape);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Softmax<math::Tensor<int8_t>>;
template class Softmax<math::Tensor<int16_t>>;
template class Softmax<math::Tensor<int32_t>>;
template class Softmax<math::Tensor<int64_t>>;
template class Softmax<math::Tensor<float>>;
template class Softmax<math::Tensor<double>>;
template class Softmax<math::Tensor<fixed_point::fp32_t>>;
template class Softmax<math::Tensor<fixed_point::fp64_t>>;
template class Softmax<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
