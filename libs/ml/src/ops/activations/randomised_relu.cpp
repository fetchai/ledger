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

#include "core/random/lfg.hpp"
#include "math/activation_functions/leaky_relu.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "ml/ops/activations/randomised_relu.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
RandomisedRelu<TensorType>::RandomisedRelu(DataType const lower_bound, DataType const upper_bound, SizeType const &random_seed)
  : lower_bound_(lower_bound)
  , upper_bound_(upper_bound)
  , bounds_mean_((upper_bound_ + lower_bound_) / DataType(2))
{
  rng_.Seed(random_seed);
  UpdateRandomValue();
}

template <typename TensorType>
RandomisedRelu<TensorType>::RandomisedRelu(SPType const &sp)
  : Ops<TensorType>(sp)
{
  lower_bound_ = sp.lower_bound;
  upper_bound_ = sp.upper_bound;
  bounds_mean_ = ((upper_bound_ + lower_bound_) / DataType(2));
  rng_.Seed(sp.random_seed);
  rng_.SetBuffer(sp.buffer);
  rng_.SetIndex(sp.index);
  random_value_ = sp.random_value;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> RandomisedRelu<TensorType>::GetOpSaveableParams()
{
  auto sp          = std::make_shared<SPType>();
  sp->lower_bound  = lower_bound_;
  sp->upper_bound  = upper_bound_;
  sp->random_seed  = rng_.Seed();
  sp->buffer       = rng_.GetBuffer();
  sp->index        = rng_.GetIndex();
  sp->random_value = random_value_;
  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> RandomisedRelu<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename TensorType>
void RandomisedRelu<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == this->ComputeOutputShape(inputs));

  if (this->is_training_)
  {
    UpdateRandomValue();
  }

  DataType alpha = this->is_training_ ? random_value_ : bounds_mean_;
  fetch::math::LeakyRelu((*inputs.front()), alpha, output);
}

template <typename TensorType>
std::vector<TensorType> RandomisedRelu<TensorType>::Backward(VecTensorType const &inputs,
                                                  TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.front()->shape() == error_signal.shape());
  DataType   zero{0};
  DataType   one{1};
  TensorType ret{error_signal.shape()};
  TensorType t{inputs.front()->shape()};

  DataType alpha = this->is_training_ ? random_value_ : bounds_mean_;

  // gradient of randomized-relu function is for x<0 = alpha, x>=0 = 1.0
  this->Forward(inputs, t);

  auto it  = t.cbegin();
  auto rit = ret.begin();
  while (it.is_valid())
  {
    if (*it >= zero)
    {
      // f'(x)=1 for x>=0
      *rit = one;
    }
    else
    {
      // f'(x)=a for x<0
      *rit = alpha;
    }
    ++it;
    ++rit;
  }

  // multiply by error_signal (chain rule)
  fetch::math::Multiply(error_signal, ret, ret);

  return {ret};
}

template <typename TensorType>
std::vector<math::SizeType> RandomisedRelu<TensorType>::ComputeOutputShape(VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class RandomisedRelu<math::Tensor<int8_t>>;
template class RandomisedRelu<math::Tensor<int16_t>>;
template class RandomisedRelu<math::Tensor<int32_t>>;
template class RandomisedRelu<math::Tensor<int64_t>>;
template class RandomisedRelu<math::Tensor<uint8_t>>;
template class RandomisedRelu<math::Tensor<uint16_t>>;
template class RandomisedRelu<math::Tensor<uint32_t>>;
template class RandomisedRelu<math::Tensor<uint64_t>>;
template class RandomisedRelu<math::Tensor<float>>;
template class RandomisedRelu<math::Tensor<double>>;
template class RandomisedRelu<math::Tensor<fixed_point::fp32_t>>;
template class RandomisedRelu<math::Tensor<fixed_point::fp64_t>>;
template class RandomisedRelu<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
