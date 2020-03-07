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

#include "math/activation_functions/sigmoid.hpp"
#include "math/fundamental_operators.hpp"
#include "math/standard_functions/exp.hpp"
#include "math/standard_functions/log.hpp"
#include "ml/ops/activations/logsigmoid.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
LogSigmoid<TensorType>::LogSigmoid(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> LogSigmoid<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> LogSigmoid<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void LogSigmoid<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  fetch::math::Sigmoid((*inputs.front()), output);
  fetch::math::Log(output, output);

  // ensures numerical stability
  for (auto &val : output)
  {
    val = fetch::vectorise::Min(val, epsilon_);
  }
}

template <typename TensorType>
std::vector<TensorType> LogSigmoid<TensorType>::Backward(VecTensorType const &inputs,
                                                         TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.front()->shape() == error_signal.shape());
  TensorType return_signal{error_signal.shape()};

  // gradient of log-sigmoid function is 1/(e^x + 1))
  fetch::math::Add(fetch::math::Exp((*inputs.front())), DataType{1}, return_signal);
  fetch::math::Divide(DataType{1}, return_signal, return_signal);

  // multiply by error_signal (chain rule)
  fetch::math::Multiply(error_signal, return_signal, return_signal);

  return {return_signal};
}

template <typename TensorType>
std::vector<math::SizeType> LogSigmoid<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  return inputs.front();
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> LogSigmoid<TensorType>::ChargeForward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_input_shapes_.empty());

  OperationsCount op_cnt = fetch::ml::charge_estimation::ops::LOG_SIGMOID_PER_ELEMENT *
                           TensorType::SizeFromShape(input_shapes[0]);

  auto output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(op_cnt, output_shape);
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> LogSigmoid<TensorType>::ChargeBackward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::LOG_SIGMOID_BACKWARD_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  math::SizeVector output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(cost * output_shape.back(), output_shape);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class LogSigmoid<math::Tensor<int8_t>>;
template class LogSigmoid<math::Tensor<int16_t>>;
template class LogSigmoid<math::Tensor<int32_t>>;
template class LogSigmoid<math::Tensor<int64_t>>;
template class LogSigmoid<math::Tensor<float>>;
template class LogSigmoid<math::Tensor<double>>;
template class LogSigmoid<math::Tensor<fixed_point::fp32_t>>;
template class LogSigmoid<math::Tensor<fixed_point::fp64_t>>;
template class LogSigmoid<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
