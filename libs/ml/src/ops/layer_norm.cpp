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
#include "math/standard_functions/pow.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "ml/ops/layer_norm.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
LayerNorm<TensorType>::LayerNorm(SizeType axis, DataType epsilon)
  : axis_(axis)
  , epsilon_(epsilon)
{}

template <typename TensorType>
LayerNorm<TensorType>::LayerNorm(SPType const &sp)
  : Ops<TensorType>(sp)
{
  epsilon_ = sp.epsilon;
  axis_    = sp.axis;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> LayerNorm<TensorType>::GetOpSaveableParams()
{
  auto sp     = std::make_shared<SPType>();
  sp->epsilon = epsilon_;
  sp->axis    = axis_;
  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> LayerNorm<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType
  copyshare->prev_input_          = TensorType(prev_input_.shape());
  copyshare->cached_inv_sqrt_var_ = TensorType(cached_inv_sqrt_var_.shape());
  copyshare->cached_output_       = TensorType(cached_output_.shape());

  return copyshare;
}

template <class TensorType>
void LayerNorm<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  // cache this inputs
  prev_input_ = *(inputs.front());

  // the layernorm can only be applied on the first axis
  assert(inputs.size() == 1);
  assert(output.shape() == inputs.front()->shape());

  // do normalization along axis = 0
  // recenter input
  TensorType mu  = fetch::math::ReduceMean(*(inputs.front()), axis_);
  cached_output_ = fetch::math::Subtract(*(inputs.front()), mu);

  // get variance of input
  TensorType sq_dev    = fetch::math::Square(cached_output_);
  cached_inv_sqrt_var_ = fetch::math::ReduceMean(sq_dev, axis_);
  fetch::math::Add(cached_inv_sqrt_var_, epsilon_, cached_inv_sqrt_var_);

  // normalize input
  fetch::math::Sqrt(cached_inv_sqrt_var_, cached_inv_sqrt_var_);
  fetch::math::Divide(cached_output_, cached_inv_sqrt_var_, cached_output_);
  fetch::math::Divide(DataType{1}, cached_inv_sqrt_var_, cached_inv_sqrt_var_);

  output = cached_output_;
}

template <class TensorType>
std::vector<TensorType> LayerNorm<TensorType>::Backward(VecTensorType const &inputs,
                                                        TensorType const &   error_signal)
{
  // refer to https://kevinzakka.github.io/2016/09/14/batch_normalization/ for detailed
  // derivation of gradient N.B. gradient for batch norm and layer norm is the same, apart from
  // the change of axis.

  // make sure we have run forward for this inputs
  if (prev_input_ != *(inputs.front()))
  {
    // if this is a new input, run the forward again
    cached_output_.Reshape(inputs.front()->shape());
    Forward(inputs, cached_output_);
  }

  // do the backward
  // 1.0 / N * ivar * (N * dxhat - np.sum(dxhat, axis=0) - xhat * np.sum(dxhat * xhat, axis=0))
  // where N = feature_length, dxhat = error_signal, xhat = cached_output_
  TensorType output_error_signal;
  auto       feature_length = static_cast<DataType>(inputs.front()->shape()[axis_]);
  TensorType dmu_dx         = fetch::math::Multiply(error_signal, feature_length);
  TensorType dout_dx        = fetch::math::ReduceSum(error_signal, axis_);
  TensorType dvar_dx =
      cached_output_ *
      fetch::math::ReduceSum(fetch::math::Multiply(error_signal, cached_output_), axis_);
  output_error_signal = cached_inv_sqrt_var_ / feature_length * (dmu_dx - dout_dx - dvar_dx);

  return {output_error_signal};
}

template <class TensorType>
std::vector<math::SizeType> LayerNorm<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  return inputs.at(0)->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class LayerNorm<math::Tensor<int8_t>>;
template class LayerNorm<math::Tensor<int16_t>>;
template class LayerNorm<math::Tensor<int32_t>>;
template class LayerNorm<math::Tensor<int64_t>>;
template class LayerNorm<math::Tensor<uint8_t>>;
template class LayerNorm<math::Tensor<uint16_t>>;
template class LayerNorm<math::Tensor<uint32_t>>;
template class LayerNorm<math::Tensor<uint64_t>>;
template class LayerNorm<math::Tensor<float>>;
template class LayerNorm<math::Tensor<double>>;
template class LayerNorm<math::Tensor<fixed_point::fp32_t>>;
template class LayerNorm<math::Tensor<fixed_point::fp64_t>>;
template class LayerNorm<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
