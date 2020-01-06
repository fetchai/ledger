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

#include "math/tensor/tensor.hpp"
#include "math/top_k.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/ops/top_k.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

/**
 * TopK function based on tf.top_k
 * @param k number of k highest numbers to be outputed
 * @param sorted TRUE=descending order, FALSE=ascending order
 */
template <typename TensorType>
TopK<TensorType>::TopK(SizeType k, bool sorted)
  : k_(k)
  , sorted_(sorted)
{}

template <typename TensorType>
TopK<TensorType>::TopK(SPType const &sp)
  : Ops<TensorType>(sp)
{
  k_      = sp.k;
  sorted_ = sp.sorted;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> TopK<TensorType>::GetOpSaveableParams()
{
  SPType sp{};
  sp.k      = k_;
  sp.sorted = sorted_;

  return std::make_shared<SPType>(sp);
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> TopK<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * Returns array of k-highest values
 * for input array of shape [x,n] and value k, return array would be of shape [k,n]
 * Implementation based on tf.math.top_k
 * Updates indices array with indices of k highest values from input array
 * @param inputs input tensor
 * @param output return tensor
 */
template <class TensorType>
void TopK<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);

  // Only 2D input is supported
  assert(inputs.at(0)->shape().size() == 2);
  assert(output.shape() == this->ComputeOutputShape(inputs));

  UpdateIndices(inputs);

  fetch::math::TopK<TensorType, TensorSizeType>(output, indices_, *(inputs.at(0)), k_, axis_,
                                                sorted_);
}

/**
 * Error signal is propagated to k largest nodes from input tensor
 * Forward needs to be called first to initialise indices array
 * @param inputs input tensor
 * @param error_signal
 * @return return signal tensor of same size as input tensor
 */
template <class TensorType>
std::vector<TensorType> TopK<TensorType>::Backward(VecTensorType const &inputs,
                                                   TensorType const &   error_signal)
{
  assert(inputs.size() == 1);

  // Only 2D input is supported
  assert(inputs.at(0)->shape().size() == 2);

  // Forward needs to be run first
  assert(indices_.size() != 0);

  assert(error_signal.shape() == this->ComputeOutputShape(inputs));

  TensorType ret_signal(inputs.at(0)->shape());

  for (SizeType i{0}; i < error_signal.shape().at(0); i++)
  {
    for (SizeType j{0}; j < error_signal.shape().at(1); j++)
    {
      ret_signal.At(indices_.At(i, j), j) = error_signal.At(i, j);
    }
  }

  return {ret_signal};
}

template <class TensorType>
std::vector<math::SizeType> TopK<TensorType>::ComputeOutputShape(VecTensorType const &inputs) const
{
  assert(inputs.size() == 1);

  std::vector<SizeType> ret_shape = inputs.at(0)->shape();

  if (ret_shape.size() > 1)
  {
    ret_shape.at(ret_shape.size() - 2) = k_;
  }
  else
  {
    ret_shape.at(ret_shape.size() - 1) = k_;
  }

  return ret_shape;
}

template <class TensorType>
void TopK<TensorType>::UpdateIndices(VecTensorType const &inputs)
{
  std::vector<SizeType> ret_shape = ComputeOutputShape(inputs);
  if (indices_.shape() != ret_shape)
  {
    indices_ = TensorSizeType(ret_shape);
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class TopK<math::Tensor<int8_t>>;
template class TopK<math::Tensor<int16_t>>;
template class TopK<math::Tensor<int32_t>>;
template class TopK<math::Tensor<int64_t>>;
template class TopK<math::Tensor<uint8_t>>;
template class TopK<math::Tensor<uint16_t>>;
template class TopK<math::Tensor<uint32_t>>;
template class TopK<math::Tensor<uint64_t>>;
template class TopK<math::Tensor<float>>;
template class TopK<math::Tensor<double>>;
template class TopK<math::Tensor<fixed_point::fp32_t>>;
template class TopK<math::Tensor<fixed_point::fp64_t>>;
template class TopK<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
