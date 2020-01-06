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

#include "math/exceptions/exceptions.hpp"
#include "math/matrix_operations.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/metrics/ops/categorical_accuracy.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
CategoricalAccuracy<TensorType>::CategoricalAccuracy(SPType const &sp)
  : Ops<T>(sp)
  , weightings_(sp.weightings)
{
  weights_sum_ = fetch::math::Sum(weightings_);
}

explicit CategoricalAccuracy(TensorType weightings)
  : weightings_(std::move(weightings))
{
  weights_sum_ = fetch::math::Sum(weightings_);
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> CategoricalAccuracy<TensorType>::GetOpSaveableParams()
{
  auto ret = std::make_shared<SPType>();

  ret->weightings = weightings_;
  return ret;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> CategoricalAccuracy<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType
  copyshare->weightings_  = weightings_.Copy();
  copyshare->weights_sum_ = weights_sum_;

  return copyshare;
}

template <typename TensorType>
void CategoricalAccuracy<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->shape() == inputs.at(1)->shape());

  auto num_correct = DataType{0};

  TensorType test_results = fetch::math::ArgMax(*inputs.at(0));
  TensorType ground_truth = fetch::math::ArgMax(*inputs.at(1));

  auto it1 = test_results.cbegin();
  auto it2 = ground_truth.cbegin();

  SizeType data_size = test_results.size();

  if (weightings_.size() == SizeType{0})
  {
    while (it1.is_valid())
    {
      if (*it1 == *it2)
      {
        ++num_correct;
      }
      ++it1;
      ++it2;
    }
    num_correct = fetch::math::Divide(num_correct, static_cast<DataType>(data_size));
  }
  // rescale according to weights
  // weighting is a batch_size vector (one weight per data point)
  else if (weightings_.shape() == std::vector<SizeType>{data_size})
  {
    auto w_it = weightings_.cbegin();
    while (it1.is_valid())
    {
      if (*it1 == *it2)
      {
        num_correct += *w_it;
      }
      ++it1;
      ++it2;
      ++w_it;
    }
    num_correct = fetch::math::Divide(num_correct, weights_sum_);
  }
  else
  {
    throw math::exceptions::WrongShape("input or weightings_ invalid");
  }

  // divide by number of elements
  output(0, 0) = num_correct;
}

/**
 * elementwise CategoricalAccuracyolute value gradient is:
 * f'(input0)=sign(input0)*error_signal
 */
template <typename TensorType>
std::vector<TensorType> CategoricalAccuracy<TensorType>::Backward(VecTensorType const &inputs,
                                                                  TensorType const &   error_signal)
{
  FETCH_UNUSED(error_signal);
  FETCH_UNUSED(inputs);

  throw fetch::ml::exceptions::NotImplemented();
}

template <typename TensorType>
std::vector<math::SizeType> CategoricalAccuracy<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  FETCH_UNUSED(inputs);
  return {1, 1};
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class CategoricalAccuracy<math::Tensor<int8_t>>;
template class CategoricalAccuracy<math::Tensor<int16_t>>;
template class CategoricalAccuracy<math::Tensor<int32_t>>;
template class CategoricalAccuracy<math::Tensor<int64_t>>;
template class CategoricalAccuracy<math::Tensor<uint8_t>>;
template class CategoricalAccuracy<math::Tensor<uint16_t>>;
template class CategoricalAccuracy<math::Tensor<uint32_t>>;
template class CategoricalAccuracy<math::Tensor<uint64_t>>;
template class CategoricalAccuracy<math::Tensor<float>>;
template class CategoricalAccuracy<math::Tensor<double>>;
template class CategoricalAccuracy<math::Tensor<fixed_point::fp32_t>>;
template class CategoricalAccuracy<math::Tensor<fixed_point::fp64_t>>;
template class CategoricalAccuracy<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
