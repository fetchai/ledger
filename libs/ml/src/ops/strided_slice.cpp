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

#include "ml/ops/strided_slice.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
StridedSlice<TensorType>::StridedSlice(SizeVector const &begins, SizeVector const &ends,
                                       SizeVector const &strides)
  : begins_(begins)
  , ends_(ends)
  , strides_(strides)
{
  assert(begins.size() == ends.size());

  // Correction to match tf.StridedSlice
  for (SizeType i{0}; i < ends_.size(); i++)
  {
    ends_[i] = ends[i] + 1;
  }

  if (strides.empty())
  {
    strides_ = begins;
    for (SizeType i{0}; i < strides.size(); i++)
    {
      strides_.at(i) = 1;
    }
  }
}

template <typename TensorType>
StridedSlice<TensorType>::StridedSlice(SPType const &sp)
  : Ops<TensorType>(sp)
{
  begins_  = sp.begins;
  ends_    = sp.ends;
  strides_ = sp.strides;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> StridedSlice<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  sp->begins  = begins_;
  sp->ends    = ends_;
  sp->strides = strides_;

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> StridedSlice<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  return std::make_shared<MyType>(*this);  // calls default copy constructor of MyType;
}

/**
 * Forward pass is done by assigning values in given ranges with stride size step for every
 * dimmension from larger input tensor to smaller output tensor.
 * @param inputs
 * @param output
 */
template <typename TensorType>
void StridedSlice<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  auto slice = inputs.at(0)->Slice(begins_, ends_, strides_);
  output.Assign(slice);
}

/**
 * Backward pass is done by assigning smaller error signal tensor to larger return signal tensor
 * @param inputs
 * @param error_signal
 * @return
 */
template <typename TensorType>
std::vector<TensorType> StridedSlice<TensorType>::Backward(VecTensorType const &inputs,
                                                           TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(error_signal.shape() ==
         ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  TensorType ret_error_signal_{inputs.at(0)->shape()};

  auto slice = ret_error_signal_.Slice(begins_, ends_, strides_);
  slice.Assign(error_signal);

  return {ret_error_signal_};
}

template <typename TensorType>
std::vector<math::SizeType> StridedSlice<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{

  SizeVector output_shape = inputs.front();

  // Calculate number of stride size steps from specified begin to specified end for each
  // dimension
  for (SizeType i{0}; i < begins_.size(); i++)
  {
    assert(strides_.at(i) != 0);
    assert(begins_.at(i) <= ends_.at(i));
    output_shape.at(i) = ((ends_.at(i) - begins_.at(i) - 1) / strides_.at(i)) + 1;
  }

  return output_shape;
}

template <typename TensorType>
OperationsCount StridedSlice<TensorType>::ChargeForward() const
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::SLICE_PER_ELEMENT *
                             this->TotalElementsIn({this->batch_input_shapes_}) +
                         fetch::ml::charge_estimation::ops::ASSIGN_PER_ELEMENT *
                             this->TotalElementsIn({this->batch_input_shapes_});

  return cost;
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> StridedSlice<TensorType>::ChargeBackward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_output_shape_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::SLICE_PER_ELEMENT *
                             this->TotalElementsIn({this->batch_output_shape_}) +
                         fetch::ml::charge_estimation::ops::ASSIGN_PER_ELEMENT *
                             this->TotalElementsIn({this->batch_output_shape_});

  math::SizeVector output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(cost * output_shape.back(), output_shape);
}
///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class StridedSlice<math::Tensor<int8_t>>;
template class StridedSlice<math::Tensor<int16_t>>;
template class StridedSlice<math::Tensor<int32_t>>;
template class StridedSlice<math::Tensor<int64_t>>;
template class StridedSlice<math::Tensor<float>>;
template class StridedSlice<math::Tensor<double>>;
template class StridedSlice<math::Tensor<fixed_point::fp32_t>>;
template class StridedSlice<math::Tensor<fixed_point::fp64_t>>;
template class StridedSlice<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
