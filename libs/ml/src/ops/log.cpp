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

#include "math/standard_functions/log.hpp"
#include "ml/ops/log.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Log<TensorType>::Log(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Log<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Log<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * elementwise Log
 * @param inputs vector containing one tensor which is the input tensor to Log
 * @return
 */
template <class TensorType>
void Log<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  fetch::math::Log((*inputs.at(0)), output);
}

/**
 * elementwise log gradient is 1/x * error:
 * f'(input0)= error_signal/input0
 */
template <class TensorType>
std::vector<TensorType> Log<TensorType>::Backward(VecTensorType const &inputs,
                                                  TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(error_signal.shape() ==
         ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  TensorType ret_error_signal(inputs.at(0)->shape());
  fetch::math::Divide(error_signal, (*inputs.at(0)), ret_error_signal);

  return {ret_error_signal};
}

template <class TensorType>
std::vector<math::SizeType> Log<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  return inputs.front();
}

template <typename TensorType>
OperationsCount Log<TensorType>::ChargeForward() const
{
  assert(!this->batch_input_shapes_.empty());

  OperationsCount cost = fetch::ml::charge_estimation::ops::LOG_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  return cost;
}

template <typename TensorType>
OperationsCount Log<TensorType>::ChargeBackward() const
{
  assert(!this->batch_output_shape_.empty());

  OperationsCount cost = fetch::ml::charge_estimation::ops::DIVISION_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_output_shape_});
  return cost;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Log<math::Tensor<int8_t>>;
template class Log<math::Tensor<int16_t>>;
template class Log<math::Tensor<int32_t>>;
template class Log<math::Tensor<int64_t>>;
template class Log<math::Tensor<float>>;
template class Log<math::Tensor<double>>;
template class Log<math::Tensor<fixed_point::fp32_t>>;
template class Log<math::Tensor<fixed_point::fp64_t>>;
template class Log<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
