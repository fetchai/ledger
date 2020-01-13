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
#include "math/activation_functions/softmax.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/metrics/cross_entropy.hpp"
#include "ml/ops/loss_functions/softmax_cross_entropy_loss.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
SoftmaxCrossEntropyLoss<TensorType>::SoftmaxCrossEntropyLoss(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> SoftmaxCrossEntropyLoss<TensorType>::GetOpSaveableParams()
{
  SPType sp{};
  return std::make_shared<SPType>(sp);
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>>
SoftmaxCrossEntropyLoss<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void SoftmaxCrossEntropyLoss<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  // third term may be present for specifying n_classes
  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());

  // sanity check the softmax adds up to 1
  assert(Sum(fetch::math::Softmax((*inputs.at(0))) - (DataType(inputs.at(0)->shape().at(1)))) <
         fetch::math::function_tolerance<DataType>());

  // softmax forward & then CrossEntropy
  output(0, 0) =
      fetch::math::CrossEntropyLoss(fetch::math::Softmax((*inputs.at(0))), (*inputs.at(1)));
}

template <typename TensorType>
std::vector<TensorType> SoftmaxCrossEntropyLoss<TensorType>::Backward(
    VecTensorType const &inputs, TensorType const &error_signal)
{
  FETCH_UNUSED(error_signal);

  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());

  TensorType ret({inputs.at(0)->shape()});
  fetch::math::Softmax((*inputs.at(0)), ret, 0);
  fetch::math::Subtract(ret, (*inputs.at(1)), ret);

  return {ret, ret};
}

template <typename TensorType>
std::vector<math::SizeType> SoftmaxCrossEntropyLoss<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  (void)inputs;
  return {1, 1};
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class SoftmaxCrossEntropyLoss<math::Tensor<int8_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<int16_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<int32_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<int64_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<uint8_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<uint16_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<uint32_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<uint64_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<float>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<double>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<fixed_point::fp32_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<fixed_point::fp64_t>>;
template class SoftmaxCrossEntropyLoss<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
