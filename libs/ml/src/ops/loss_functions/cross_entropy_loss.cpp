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
#include "math/metrics/cross_entropy.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
CrossEntropyLoss<TensorType>::CrossEntropyLoss(SPType const &sp)
  : Ops<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> CrossEntropyLoss<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> CrossEntropyLoss<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void CrossEntropyLoss<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());

  output(0, 0) = fetch::math::CrossEntropyLoss((*inputs.at(0)), (*inputs.at(1)));
}

template <typename TensorType>
std::vector<TensorType> CrossEntropyLoss<TensorType>::Backward(VecTensorType const &inputs,
                                                               TensorType const &   error_signal)
{
  FETCH_UNUSED(error_signal);

  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());
  assert(inputs.at(0)->shape().size() == 2);

  bool is_binary  = (inputs.at(0)->shape(0) == 1);
  auto batch_size = static_cast<DataType>(inputs.at(0)->shape(1));

  TensorType ret({inputs.at(0)->shape()});

  auto       a_it = inputs.at(0)->cbegin();
  auto       b_it = inputs.at(1)->cbegin();
  auto       r_it = ret.begin();
  auto const one  = DataType{1};

  while (a_it.is_valid())
  {
    assert(*b_it == DataType{0} || *b_it == DataType{1});
    if (*b_it == DataType{1})
    {
      *r_it = static_cast<DataType>(-*b_it / *a_it);
    }
    else if (is_binary)
    {
      *r_it = static_cast<DataType>((one - *b_it) / (one - *a_it));
    }

    ++a_it;
    ++b_it;
    ++r_it;
  }
  return {ret / batch_size, ret};
}

template <typename TensorType>
std::vector<math::SizeType> CrossEntropyLoss<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  FETCH_UNUSED(inputs);
  return {1, 1};
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> CrossEntropyLoss<TensorType>::ChargeForward(
    std::vector<math::SizeVector> const &input_shapes)
{
  auto output_shape = ComputeOutputShape(input_shapes);
  auto n_elements   = TensorType::SizeFromShape(input_shapes[0]);
  auto padded_size  = TensorType::PaddedSizeFromShape(input_shapes[0]);
  auto n_dims       = input_shapes[0].at(0);

  OperationsCount cost{fetch::ml::charge_estimation::ops::OP_OVERHEAD};
  OperationsCount iteration_ops = TensorType::ChargeIterate(output_shape);
  cost += iteration_ops * 4;

  // if not a one-hot, must be binary logistic regression cost
  if (n_dims == 1)
  {

    if ((padded_size / 32) <
        fetch::ml::charge_estimation::ops::CEL_BINARY_PIECEWISE_LOWER_THRESHOLD)
    {
      // Addition cost
      cost += fetch::ml::charge_estimation::ops::LOW_CROSS_ENTROPY_BINARY_PER_ELEMENT * n_elements;
    }
    else if ((padded_size / 32) < fetch::ml::charge_estimation::ops::PIECEWISE_HARD_CAP)
    {
      // Addition cost
      cost += fetch::ml::charge_estimation::ops::HIGH_CROSS_ENTROPY_BINARY_PER_ELEMENT * n_elements;
    }
    else
    {
      cost = math::numeric_max<OperationsCount>();
    }
  }
  else
  {
    // Addition cost
    cost += fetch::ml::charge_estimation::ops::CROSS_ENTROPY_ONE_HOT_PER_ELEMENT * n_elements;
  }

  return std::make_pair(cost, output_shape);
}

template <typename TensorType>
OperationsCount CrossEntropyLoss<TensorType>::ChargeBackward() const
{
  assert(!this->batch_input_shapes_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::CROSS_ENTROPY_BACKWARD_PER_ELEMENT *
                         this->TotalElementsIn({this->batch_input_shapes_.at(0)});
  return cost;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class CrossEntropyLoss<math::Tensor<int8_t>>;
template class CrossEntropyLoss<math::Tensor<int16_t>>;
template class CrossEntropyLoss<math::Tensor<int32_t>>;
template class CrossEntropyLoss<math::Tensor<int64_t>>;
template class CrossEntropyLoss<math::Tensor<float>>;
template class CrossEntropyLoss<math::Tensor<double>>;
template class CrossEntropyLoss<math::Tensor<fixed_point::fp32_t>>;
template class CrossEntropyLoss<math::Tensor<fixed_point::fp64_t>>;
template class CrossEntropyLoss<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
