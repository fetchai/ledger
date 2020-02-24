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

#include "ml/charge_estimation/ops/constants.hpp"
#include "ml/ops/dataholder.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> DataHolder<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();
  if (this->data_)
  {
    sp->data = std::make_shared<TensorType>(this->data_->Copy());
  }
  sp->future_data_shape = future_data_shape_;

  // Add base class savable params
  auto ops_sp  = ParentClass::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<typename ParentClass::SPType>(sp);
  *cast_sp     = *(std::static_pointer_cast<typename ParentClass::SPType>(ops_sp));

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
  this->future_data_shape_ = data.shape();

  return shape_changed;
}

template <class TensorType>
std::vector<math::SizeType> DataHolder<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  FETCH_UNUSED(inputs);
  if (!future_data_shape_.empty())
  {
    return future_data_shape_;
  }

  throw std::runtime_error("future_data_shape_ is not set for " + std ::string(Descriptor()) +
                           " so shape computing is not possible.");
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

template <typename TensorType>
OperationsCount DataHolder<TensorType>::ChargeConstruct()
{
  return charge_estimation::ops::OP_DEFAULT_CONSTRUCTION_COST;
}

template <typename TensorType>
OperationsCount DataHolder<TensorType>::ChargeSetData(std::vector<SizeType> const &data)
{
  future_data_shape_ = data;
  return TensorType::PaddedSizeFromShape(data);
}

/**
 * This is used for charge estimation, where the shape of the data tensor is needed but
 * setting the full tensor would be too expensive.
 * @tparam TensorType
 * @param data
 */
template <typename TensorType>
void DataHolder<TensorType>::SetFutureDataShape(std::vector<SizeType> const &data)
{
  future_data_shape_ = data;
}

template <typename T>
DataHolder<T>::DataHolder(const DataHolder::SPType &sp)
  : Ops<T>(sp)
{
  if (sp.data)
  {
    this->data_ = std::make_shared<TensorType>(sp.data->Copy());
  }
  future_data_shape_ = sp.future_data_shape;
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
