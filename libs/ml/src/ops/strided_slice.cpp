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
  assert(output.shape() == this->ComputeOutputShape(inputs));

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
  assert(error_signal.shape() == this->ComputeOutputShape(inputs));

  TensorType ret_error_signal_{inputs.at(0)->shape()};

  auto slice = ret_error_signal_.Slice(begins_, ends_, strides_);
  slice.Assign(error_signal);

  return {ret_error_signal_};
}

template <typename TensorType>
std::vector<math::SizeType> StridedSlice<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{

  SizeVector output_shape = inputs.front()->shape();

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

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class StridedSlice<math::Tensor<int8_t>>;
template class StridedSlice<math::Tensor<int16_t>>;
template class StridedSlice<math::Tensor<int32_t>>;
template class StridedSlice<math::Tensor<int64_t>>;
template class StridedSlice<math::Tensor<uint8_t>>;
template class StridedSlice<math::Tensor<uint16_t>>;
template class StridedSlice<math::Tensor<uint32_t>>;
template class StridedSlice<math::Tensor<uint64_t>>;
template class StridedSlice<math::Tensor<float>>;
template class StridedSlice<math::Tensor<double>>;
template class StridedSlice<math::Tensor<fixed_point::fp32_t>>;
template class StridedSlice<math::Tensor<fixed_point::fp64_t>>;
template class StridedSlice<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
