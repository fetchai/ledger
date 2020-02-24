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
#include "ml/ops/add.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Add<TensorType>::Add(SPType const &sp)
  : Ops<TensorType>(sp)
{
  axes_ = sp.axes;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Add<TensorType>::GetOpSaveableParams()
{
  auto sp  = std::make_shared<SPType>();
  sp->axes = axes_;

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Add<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void Add<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(output.shape() == Ops<TensorType>::ComputeOutputShape(inputs));
  fetch::math::Add((*inputs.at(0)), (*inputs.at(1)), output);
}

template <typename TensorType>
std::vector<TensorType> Add<TensorType>::Backward(VecTensorType const &inputs,
                                                  TensorType const &   error_signal)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->shape().size() == inputs.at(1)->shape().size());
  assert(inputs.at(0)->shape() == error_signal.shape());
  assert(error_signal.shape() == Ops<TensorType>::ComputeOutputShape(inputs));

  if (inputs.at(0)->shape() == inputs.at(1)->shape())
  {
    // Non-broadcast Add
    return {error_signal, error_signal};
  }

  // Broadcast Add
  UpdateAxes(inputs);
  return {error_signal, fetch::math::ReduceSum(error_signal, axes_)};
}

template <typename TensorType>
std::vector<math::SizeType> Add<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  return inputs.at(0);
}

template <typename TensorType>
OpType Add<TensorType>::OperationType() const
{
  return this->OpCode();
}

template <typename TensorType>
const char *Add<TensorType>::Descriptor() const
{
  return DESCRIPTOR;
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> Add<TensorType>::ChargeForward(
    std::vector<math::SizeVector> input_shapes)
{
  assert(!this->batch_output_shape_.empty());

  OperationsCount op_cnt{1};

  auto output_shape = ComputeOutputShape(input_shapes);

  // Addition cost
  SizeType num_elements = TensorType::SizeFromShape(output_shape);
  op_cnt += num_elements;

  // Iteration over 3 tensors (input1, input2, ret)
  OperationsCount iteration_ops = TensorType::ChargeIterate(output_shape);
  op_cnt += iteration_ops * 3;

  return std::make_pair(op_cnt, output_shape);
}

template <typename TensorType>
OperationsCount Add<TensorType>::ChargeBackward() const
{
  assert(!this->batch_output_shape_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::LOW_ADDITION_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_output_shape_});
  return cost;
}

/**
 * method for updating axes in case of broadcast Add
 * @tparam TensorType
 * @param inputs
 */
template <typename TensorType>
void Add<TensorType>::UpdateAxes(VecTensorType const &inputs)
{
  bool axes_changed = false;

  // Check if axes were changed
  SizeType cnt = 0;
  for (SizeType i{0}; i < inputs.at(0)->shape().size(); i++)
  {
    if (inputs.at(0)->shape().at(i) != inputs.at(1)->shape().at(i))
    {
      if (cnt >= axes_.size() || axes_.at(cnt) != i)
      {
        axes_changed = true;
        break;
      }
      cnt++;
    }
  }

  if (axes_.empty())
  {
    axes_changed = true;
  }

  // Update axes if necessary
  if (axes_changed)
  {
    axes_.clear();
    // Get axes
    for (SizeType i{0}; i < inputs.at(0)->shape().size(); i++)
    {
      if (inputs.at(0)->shape().at(i) != inputs.at(1)->shape().at(i))
      {
        axes_.emplace_back(i);
      }
    }
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Add<math::Tensor<int8_t>>;
template class Add<math::Tensor<int16_t>>;
template class Add<math::Tensor<int32_t>>;
template class Add<math::Tensor<int64_t>>;
template class Add<math::Tensor<float>>;
template class Add<math::Tensor<double>>;
template class Add<math::Tensor<fixed_point::fp32_t>>;
template class Add<math::Tensor<fixed_point::fp64_t>>;
template class Add<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
