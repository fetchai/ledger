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
#include "ml/ops/reshape.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Reshape<TensorType>::Reshape(std::vector<SizeType> new_shape)
  : new_shape_(std::move(new_shape))
  , new_size_(fetch::math::Product(new_shape_))
{
  assert(new_shape_.size() > 1);
}

template <typename TensorType>
Reshape<TensorType>::Reshape(SPType const &sp)
  : Ops<TensorType>(sp)
  , new_shape_(sp.new_shape)
  , new_size_(sp.new_size)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Reshape<TensorType>::GetOpSaveableParams()
{
  SPType sp{};
  sp.new_shape = new_shape_;
  sp.new_size  = new_size_;
  return std::make_shared<SPType>(sp);
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Reshape<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);
  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType
  return copyshare;
}

template <typename TensorType>
void Reshape<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == ComputeOutputShape(inputs));

  // if batch sizes don't agree - update specified new_shape
  SizeType input_batch_size = inputs.at(0)->shape(inputs.at(0)->shape().size() - 1);
  SizeType new_batch_size   = new_shape_.at(new_shape_.size() - 1);
  if (input_batch_size != new_batch_size)
  {
    new_shape_.at(new_shape_.size() - 1) = input_batch_size;
    new_size_                            = fetch::math::Product(new_shape_);
  }

  // if the shape is exactly the same just copy the data
  if ((*inputs.front()).shape() == new_shape_)
  {
    output.Assign((*inputs.front()));
  }
  // check the reshape sizes match!
  else if ((*inputs.front()).size() != new_size_)
  {
    throw fetch::math::exceptions::WrongShape(
        "new shape has different size from current tensor size");
  }
  else
  {
    output.Reshape(new_shape_);
    output.Assign((*inputs.front()));
  }
}

template <typename TensorType>
std::vector<TensorType> Reshape<TensorType>::Backward(VecTensorType const &inputs,
                                                      TensorType const &   error_signal)
{
  assert(inputs.size() == 1);

  TensorType ret(inputs.at(0)->shape());
  ret.Assign(error_signal);
  return {ret};
}

template <typename TensorType>
std::vector<math::SizeType> Reshape<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  assert(inputs.size() == 1);
  assert(inputs.at(0)->shape().size() > 1);

  // output shape prespecified (except batch dimension)
  std::vector<SizeType> output_shape = new_shape_;

  // overwrite the batch dimension
  output_shape.at(output_shape.size() - 1) =
      inputs.at(0)->shape().at(inputs.at(0)->shape().size() - 1);

  return output_shape;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Reshape<math::Tensor<int8_t>>;
template class Reshape<math::Tensor<int16_t>>;
template class Reshape<math::Tensor<int32_t>>;
template class Reshape<math::Tensor<int64_t>>;
template class Reshape<math::Tensor<uint8_t>>;
template class Reshape<math::Tensor<uint16_t>>;
template class Reshape<math::Tensor<uint32_t>>;
template class Reshape<math::Tensor<uint64_t>>;
template class Reshape<math::Tensor<float>>;
template class Reshape<math::Tensor<double>>;
template class Reshape<math::Tensor<fixed_point::fp32_t>>;
template class Reshape<math::Tensor<fixed_point::fp64_t>>;
template class Reshape<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
