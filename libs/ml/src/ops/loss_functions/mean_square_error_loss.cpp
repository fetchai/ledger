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
#include "math/metrics/mean_square_error.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
MeanSquareErrorLoss<TensorType>::MeanSquareErrorLoss(SPType const &sp)
  : Ops<TensorType>(sp)
  , weightings_(sp.weightings)
{}

template <typename TensorType>
MeanSquareErrorLoss<TensorType>::MeanSquareErrorLoss(TensorType weightings)
  : weightings_(std::move(weightings))
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> MeanSquareErrorLoss<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  sp->weightings = weightings_;

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MeanSquareErrorLoss<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType
  copyshare->weightings_ = weightings_.Copy();

  return copyshare;
}

template <typename TensorType>
void MeanSquareErrorLoss<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->shape() == inputs.at(1)->shape());

  if (weightings_.size() == static_cast<SizeType>(0))
  {
    *(output.begin()) = fetch::math::MeanSquareError(*(inputs.at(0)), *(inputs.at(1)));
  }
  // rescale according to weights
  else
  {
    SizeType data_size = inputs.at(0)->shape(inputs.at(0)->shape().size() - 1);

    auto it1 = inputs.at(0)->cbegin();
    auto it2 = inputs.at(1)->cbegin();

    // weighting is scalar
    if (weightings_.shape().size() == static_cast<SizeType>(1))
    {
      while (it1.is_valid())
      {
        auto d       = static_cast<DataType>((*it1) - (*it2));
        output(0, 0) = static_cast<DataType>(output(0, 0) + (d * d) * weightings_(0));
        ++it1;
        ++it2;
      }
    }
    // weighting tensor is same shape as input (one weight for every parameter)
    else if (weightings_.shape() == inputs.at(0)->shape())
    {
      auto w_it = weightings_.cbegin();
      while (it1.is_valid())
      {
        auto d       = static_cast<DataType>((*it1) - (*it2));
        output(0, 0) = static_cast<DataType>(output(0, 0) + (d * d) * (*w_it));

        ++it1;
        ++it2;
        ++w_it;
      }
    }
    // weighting is a batch_size vector (one weight per data point)
    else if (weightings_.shape() == std::vector<SizeType>{data_size})
    {
      SizeType data_count = 0;
      SizeType data_stride;
      fetch::math::Divide(inputs.at(0)->size(), weightings_.size(), data_stride);

      auto w_it = weightings_.cbegin();
      while (it1.is_valid())
      {
        auto d       = static_cast<DataType>((*it1) - (*it2));
        output(0, 0) = static_cast<DataType>(output(0, 0) + (d * d) * (*w_it));
        ++it1;
        ++it2;

        ++data_count;
        if (data_count == data_stride)
        {
          data_count = 0;
          ++w_it;
        }
      }
    }

    // divide by number of elements
    fetch::math::Divide(output(0, 0), static_cast<DataType>(inputs.at(0)->size()), output(0, 0));
  }
}

/**
 * Gradients for Mean Square Error Loss would usually be of the form:
 * grad[0] = 2 * err * (in[0] - in[1]) / data_size
 * grad[1] = -2 * err * (in[0] - in[1]) / data_size
 *
 * However we make a few alterations:
 * 1. we ignore the gradient for the ground truth (i.e. grad[1]),
 * 2. we must incorporate the weightings,
 *
 * so the modified gradient is computed as:
 * grad[0] = 2 * err * (in[0] - in[1])  * weighting / data_size
 * grad[1] = grad[0] -- SHOULD NOT BE USED
 *
 * @param inputs vector of input_tensor and ground_truth tensor (order is important)
 * @param error_signal error_signal passed back from next layer - since there should not be a next
 * layer the calling function is required to set this to a tensor of size 1 and value 1
 * @return
 */
template <typename TensorType>
std::vector<TensorType> MeanSquareErrorLoss<TensorType>::Backward(VecTensorType const &inputs,
                                                                  TensorType const &   error_signal)
{
  FETCH_UNUSED(error_signal);

  assert(inputs.size() == 2);
  assert(inputs.at(0)->shape() == inputs.at(1)->shape());

  TensorType return_signal(inputs.front()->shape());

  SizeType data_size = inputs.at(0)->shape(inputs.at(0)->shape().size() - 1);
  auto     count     = static_cast<DataType>(data_size);

  // backprop update rule varies depending on shape of weightings
  auto a_it = inputs.at(0)->cbegin();
  auto b_it = inputs.at(1)->cbegin();
  auto r_it = return_signal.begin();

  // no weighting
  if (weightings_.size() == static_cast<SizeType>(0))
  {
    while (r_it.is_valid())
    {
      *r_it = static_cast<DataType>(((*a_it) - (*b_it)) / count);
      ++a_it;
      ++b_it;
      ++r_it;
    }
  }
  else
  {
    // weighting is scalar
    if (weightings_.shape().size() == static_cast<SizeType>(1))
    {
      auto weight_over_count = weightings_(0) / count;
      while (r_it.is_valid())
      {
        *r_it = static_cast<DataType>(((*a_it) - (*b_it)) * weight_over_count);
        ++a_it;
        ++b_it;
        ++r_it;
      }
    }
    // weighting tensor is same shape as input (one weight for every parameter)
    else if (weightings_.shape() == inputs.at(0)->shape())
    {
      auto w_it = weightings_.cbegin();
      while (r_it.is_valid())
      {
        *r_it = static_cast<DataType>((((*a_it) - (*b_it)) * (*w_it)) / (count));

        ++a_it;
        ++b_it;
        ++r_it;
        ++w_it;
      }
    }
    // weighting is a batch_size vector (one weight per data point)
    else if (weightings_.shape() == std::vector<SizeType>{data_size})
    {
      auto     w_it       = weightings_.cbegin();
      SizeType data_count = 0;
      SizeType data_stride;
      fetch::math::Divide(inputs.at(0)->size(), weightings_.size(), data_stride);
      while (r_it.is_valid())
      {
        *r_it = static_cast<DataType>((((*a_it) - (*b_it)) * (*w_it)) / (count));
        ++a_it;
        ++b_it;
        ++r_it;

        // update weight value once per data point
        ++data_count;
        if (data_count == data_stride)
        {
          data_count = 0;
          ++w_it;
        }
      }
    }
    else
    {
      throw math::exceptions::WrongShape("input or weightings_shape invalid");
    }
  }

  fetch::math::Multiply(return_signal, static_cast<DataType>(2), return_signal);

  return {return_signal, return_signal};
}

template <typename TensorType>
std::vector<math::SizeType> MeanSquareErrorLoss<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  FETCH_UNUSED(inputs);
  return {1, 1};
}

template <typename TensorType>
OperationsCount MeanSquareErrorLoss<TensorType>::ChargeForward() const
{
  assert(!this->batch_output_shape_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::MEAN_SQ_ERROR_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_});
  return cost;
}

template <typename TensorType>
OperationsCount MeanSquareErrorLoss<TensorType>::ChargeBackward() const
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::MEAN_SQ_ERROR_BACKWARD_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_.at(0)});
  return cost;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class MeanSquareErrorLoss<math::Tensor<int8_t>>;
template class MeanSquareErrorLoss<math::Tensor<int16_t>>;
template class MeanSquareErrorLoss<math::Tensor<int32_t>>;
template class MeanSquareErrorLoss<math::Tensor<int64_t>>;
template class MeanSquareErrorLoss<math::Tensor<float>>;
template class MeanSquareErrorLoss<math::Tensor<double>>;
template class MeanSquareErrorLoss<math::Tensor<fixed_point::fp32_t>>;
template class MeanSquareErrorLoss<math::Tensor<fixed_point::fp64_t>>;
template class MeanSquareErrorLoss<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
