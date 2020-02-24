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
#include "math/standard_functions/log.hpp"
#include "ml/ops/activations/logsoftmax.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
LogSoftmax<TensorType>::LogSoftmax(SizeType axis)
  : axis_(axis)
{}

template <typename TensorType>
LogSoftmax<TensorType>::LogSoftmax(SPType const &sp)
  : Ops<TensorType>(sp)
{
  axis_ = sp.axis;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> LogSoftmax<TensorType>::GetOpSaveableParams()
{
  auto sp  = std::make_shared<SPType>();
  sp->axis = axis_;

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> LogSoftmax<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void LogSoftmax<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));
  assert(inputs.size() == 1);
  fetch::math::Softmax((*inputs.front()), output, axis_);
  fetch::math::Log(output, output);
}

template <typename TensorType>
std::vector<TensorType> LogSoftmax<TensorType>::Backward(VecTensorType const &inputs,
                                                         TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.front()->shape() == error_signal.shape());

  TensorType return_signal = error_signal.Copy();
  TensorType t(error_signal.shape());
  fetch::math::Softmax((*inputs.front()), t, axis_);

  // N-D softmax with 1 batch dimension
  if (inputs.front()->shape().size() > 1)
  {
    TensorType sum = ReduceSum(return_signal, axis_);
    t.InlineMultiply(sum);
  }

  return_signal.InlineSubtract(t);
  return {return_signal};
}

template <typename TensorType>
std::vector<math::SizeType> LogSoftmax<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  return inputs.front();
}

template <typename TensorType>
OperationsCount LogSoftmax<TensorType>::ChargeForward() const
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::LOG_SOFTMAX_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  return cost;
}

template <typename TensorType>
OperationsCount LogSoftmax<TensorType>::ChargeBackward() const
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::LOG_SOFTMAX_BACKWARD_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  return cost;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class LogSoftmax<math::Tensor<int8_t>>;
template class LogSoftmax<math::Tensor<int16_t>>;
template class LogSoftmax<math::Tensor<int32_t>>;
template class LogSoftmax<math::Tensor<int64_t>>;
template class LogSoftmax<math::Tensor<float>>;
template class LogSoftmax<math::Tensor<double>>;
template class LogSoftmax<math::Tensor<fixed_point::fp32_t>>;
template class LogSoftmax<math::Tensor<fixed_point::fp64_t>>;
template class LogSoftmax<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
