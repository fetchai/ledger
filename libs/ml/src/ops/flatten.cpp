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

#include "math/matrix_operations.hpp"
#include "ml/charge_estimation/ops/constants.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Flatten<TensorType>::Flatten(SPType const &sp)
  : Ops<TensorType>(sp)
{
  input_shape_ = sp.input_shape;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Flatten<TensorType>::GetOpSaveableParams()
{
  auto sp         = std::make_shared<SPType>();
  sp->input_shape = input_shape_;

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Flatten<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <class TensorType>
void Flatten<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == ComputeOutputShape(inputs));
  input_shape_ = inputs.front()->shape();

  assert(output.shape().at(output.shape().size() - 1) ==
         inputs.front()->shape().at(inputs.front()->shape().size() - 1));
  output.Assign(inputs.front()->View());
}

template <class TensorType>
std::vector<TensorType> Flatten<TensorType>::Backward(VecTensorType const &inputs,
                                                      TensorType const &   error_signal)
{
  FETCH_UNUSED(inputs);
  assert(inputs.size() == 1);
  TensorType ret(input_shape_);

  assert(ret.shape().at(ret.shape().size() - 1) ==
         error_signal.shape().at(error_signal.shape().size() - 1));
  ret.Assign(error_signal.View());

  return {ret};
}

template <class TensorType>
std::vector<math::SizeType> Flatten<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  SizeType batch_size = inputs.at(0)->shape().at(inputs.at(0)->shape().size() - SizeType{1});
  SizeType data_size  = 1;
  for (SizeType i{0}; i < inputs.at(0)->shape().size() - SizeType{1}; i++)
  {
    data_size *= inputs.at(0)->shape().at(i);
  }

  return {data_size, batch_size};
}

template <class TensorType>
OpType Flatten<TensorType>::OperationType() const
{
  return this->OpCode();
}

template <class TensorType>
const char *Flatten<TensorType>::Descriptor() const
{
  return DESCRIPTOR;
}

template <class TensorType>
OperationsCount Flatten<TensorType>::ChargeForward() const
{
  assert(!this->batch_input_shapes_.empty());

  OperationsCount cost = fetch::ml::charge_estimation::ops::OP_OVERHEAD;

  auto n_elements = this->TotalElementsIn(this->batch_input_shapes_);

  if (n_elements < fetch::ml::charge_estimation::ops::PIECEWISE_LOWER_THRESHOLD)
  {
    cost += fetch::ml::charge_estimation::ops::LOW_FLATTEN_PER_ELEMENT *
            this->TotalElementsIn(this->batch_input_shapes_);
  }
  else if (n_elements < fetch::ml::charge_estimation::ops::PIECEWISE_HARD_CAP)
  {
    cost += fetch::ml::charge_estimation::ops::HIGH_FLATTEN_PER_ELEMENT *
            this->TotalElementsIn(this->batch_input_shapes_);
  }
  else
  {
    cost += math::numeric_max<OperationsCount>();
  }

  return cost;
}

template <class TensorType>
OperationsCount Flatten<TensorType>::ChargeBackward() const
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::RESHAPE_PER_ELEMENT *
                             this->TotalElementsIn(this->batch_input_shapes_) +
                         fetch::ml::charge_estimation::ops::ASSIGN_PER_ELEMENT *
                             this->TotalElementsIn(this->batch_input_shapes_);
  return cost;
}

template <class TensorType>
OperationsCount Flatten<TensorType>::ChargeConstruct()
{
  return charge_estimation::ops::OP_DEFAULT_CONSTRUCTION_COST;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Flatten<math::Tensor<int8_t>>;
template class Flatten<math::Tensor<int16_t>>;
template class Flatten<math::Tensor<int32_t>>;
template class Flatten<math::Tensor<int64_t>>;
template class Flatten<math::Tensor<float>>;
template class Flatten<math::Tensor<double>>;
template class Flatten<math::Tensor<fixed_point::fp32_t>>;
template class Flatten<math::Tensor<fixed_point::fp64_t>>;
template class Flatten<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
