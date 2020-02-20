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

#include "ml/ops/dataholder.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> DataHolder<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

/**
 * forward recovers the stored data
 * @param inputs
 * @param output
 */
template <class TensorType>
void DataHolder<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  FETCH_UNUSED(inputs);
  assert(inputs.empty());
  assert(data_);
  output = *(data_);
}

/**
 * backward for non training dataholders should just pass back the error signal
 * @param inputs
 * @param error_signal
 * @return
 */
template <class TensorType>
std::vector<TensorType> DataHolder<TensorType>::Backward(VecTensorType const &inputs,
                                                         TensorType const &   error_signal)
{
  FETCH_UNUSED(inputs);
  assert(inputs.empty());
  return {error_signal};
}

/**
 * sets the internally stored data, and returns a bool indicating whether the shape has changed
 * @param data
 * @return
 */
template <class TensorType>
bool DataHolder<TensorType>::SetData(TensorType const &data)
{
  bool shape_changed = (data_->shape() != data.shape());
  data_->Copy(data);
  return shape_changed;
}

template <class TensorType>
std::vector<math::SizeType> DataHolder<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  FETCH_UNUSED(inputs);
  if (!data_)
  {
    throw std::runtime_error("Data is not set for " + std ::string(Descriptor()) +
                             ", shape computing is not possible.");
  }
  return data_->shape();
}

template <typename TensorType>
OperationsCount DataHolder<TensorType>::ChargeForward() const
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::ASSIGN_PER_ELEMENT;

  return cost;
}

template <typename TensorType>
OperationsCount DataHolder<TensorType>::ChargeBackward() const
{
  OperationsCount cost = 0;

  return cost;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class DataHolder<math::Tensor<int8_t>>;
template class DataHolder<math::Tensor<int16_t>>;
template class DataHolder<math::Tensor<int32_t>>;
template class DataHolder<math::Tensor<int64_t>>;
template class DataHolder<math::Tensor<float>>;
template class DataHolder<math::Tensor<double>>;
template class DataHolder<math::Tensor<fixed_point::fp32_t>>;
template class DataHolder<math::Tensor<fixed_point::fp64_t>>;
template class DataHolder<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
