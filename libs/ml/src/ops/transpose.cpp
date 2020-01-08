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

#include "ml/ops/transpose.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Transpose<TensorType>::Transpose(std::vector<SizeType> transpose_vector)
  : transpose_vector_(std::move(transpose_vector))
{}

template <typename TensorType>
Transpose<TensorType>::Transpose(SPType const &sp)
  : Ops<TensorType>(sp)
{
  transpose_vector_ = sp.transpose_vector;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Transpose<TensorType>::GetOpSaveableParams()
{
  SPType sp{};
  sp.transpose_vector = transpose_vector_;
  return std::make_shared<SPType>(sp);
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Transpose<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <class TensorType>
void Transpose<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == this->ComputeOutputShape(inputs));

  if (inputs.front()->shape().size() == 2)
  {
    output.Copy(inputs.front()->Transpose());
  }
  else
  {
    output.Copy(inputs.front()->Transpose(transpose_vector_));
  }
}

template <class TensorType>
std::vector<TensorType> Transpose<TensorType>::Backward(VecTensorType const &inputs,
                                                        TensorType const &   error_signal)
{
  FETCH_UNUSED(inputs);
  assert(inputs.size() == 1);
  assert(error_signal.shape() == this->ComputeOutputShape(inputs));

  if (error_signal.shape().size() == 2)
  {
    return {error_signal.Transpose()};
  }

  return {error_signal.Transpose(transpose_vector_)};
}

template <class TensorType>
std::vector<math::SizeType> Transpose<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  // 2D transpose
  if (inputs.at(0)->shape().size() == 2)
  {
    return {inputs.front()->shape().at(1), inputs.front()->shape().at(0)};
  }
  // Transpose by given vector

  std::vector<SizeType> input_shape = inputs.front()->shape();
  std::vector<SizeType> shape;

  shape.reserve(shape.size());
  for (auto &current_size : transpose_vector_)
  {
    shape.push_back(input_shape.at(current_size));
  }

  return shape;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Transpose<math::Tensor<int8_t>>;
template class Transpose<math::Tensor<int16_t>>;
template class Transpose<math::Tensor<int32_t>>;
template class Transpose<math::Tensor<int64_t>>;
template class Transpose<math::Tensor<uint8_t>>;
template class Transpose<math::Tensor<uint16_t>>;
template class Transpose<math::Tensor<uint32_t>>;
template class Transpose<math::Tensor<uint64_t>>;
template class Transpose<math::Tensor<float>>;
template class Transpose<math::Tensor<double>>;
template class Transpose<math::Tensor<fixed_point::fp32_t>>;
template class Transpose<math::Tensor<fixed_point::fp64_t>>;
template class Transpose<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
