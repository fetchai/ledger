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

#include "ml/ops/embeddings.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Embeddings<TensorType>::Embeddings(SizeType dimensions, SizeType data_points)
{
  TensorType weights = TensorType(std::vector<SizeType>({dimensions, data_points}));
  fetch::ml::ops::Weights<TensorType>::Initialise(weights, dimensions, data_points);
  Weights<TensorType>::SetData(weights);
}

template <typename TensorType>
Embeddings<TensorType>::Embeddings(TensorType const &weights)
{
  Weights<TensorType>::SetData(weights);
}

template <typename TensorType>
Embeddings<TensorType>::Embeddings(SPType const &sp)
  : Weights<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Embeddings<TensorType>::GetOpSaveableParams()
{
  auto sp   = std::make_shared<SPType>();
  auto w_sp = Weights<TensorType>::GetOpSaveableParams();

  auto cast_sp = std::static_pointer_cast<OpWeightsSaveableParams<TensorType>>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpWeightsSaveableParams<TensorType>>(w_sp));

  return sp;
}

template <class TensorType>
void Embeddings<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(this->data_);
  assert(inputs.size() == 1);
  assert(inputs.front()->shape().size() == 2);

  SizeType batch_size = inputs.front()->shape().at(1);

  assert(output.shape().at(1) == inputs.front()->shape().at(0));
  assert(output.shape().at(0) == this->data_->shape().at(0));
  assert(output.shape().at(2) == batch_size);

  auto indices  = inputs.front()->shape().at(0);
  auto input_it = inputs.front()->begin();
  for (SizeType i{0}; i < indices; i++)
  {
    for (SizeType n{0}; n < batch_size; n++)
    {
      auto output_view    = output.View({i, n});
      auto embedding_view = this->data_->View(static_cast<SizeType>(*input_it));
      output_view.Assign(embedding_view);
      ++input_it;
    }
  }
}

template <class TensorType>
std::vector<TensorType> Embeddings<TensorType>::Backward(VecTensorType const &inputs,
                                                         TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(inputs.front()->shape().size() == 2);

  if (!this->value_frozen_)
  {
    SizeType batch_size = inputs.front()->shape(1);

    auto indices  = inputs.front()->shape().at(0);
    auto input_it = inputs.front()->begin();

    for (SizeType i{0}; i < indices; i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {
        auto error_view    = error_signal.View({i, n});
        auto gradient_view = this->gradient_accumulation_->View(static_cast<SizeType>(*input_it));

        // Mark update
        this->updated_rows_.insert(static_cast<SizeType>(*input_it));

        auto error_view_it    = error_view.cbegin();
        auto gradient_view_it = gradient_view.begin();
        while (error_view_it.is_valid())
        {
          *gradient_view_it = static_cast<DataType>(*gradient_view_it + *error_view_it);
          ++error_view_it;
          ++gradient_view_it;
        }
        ++input_it;
      }
    }

    this->reset_gradients_ = true;
  }

  return {TensorType(error_signal.shape())};
}

template <class TensorType>
std::vector<math::SizeType> Embeddings<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  auto                  feature_size = this->data_->shape().at(0);
  std::vector<SizeType> output_shape{feature_size, inputs.front()->shape().at(0),
                                     inputs.front()->shape().at(1)};
  return output_shape;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Embeddings<math::Tensor<int8_t>>;
template class Embeddings<math::Tensor<int16_t>>;
template class Embeddings<math::Tensor<int32_t>>;
template class Embeddings<math::Tensor<int64_t>>;
template class Embeddings<math::Tensor<uint8_t>>;
template class Embeddings<math::Tensor<uint16_t>>;
template class Embeddings<math::Tensor<uint32_t>>;
template class Embeddings<math::Tensor<uint64_t>>;
template class Embeddings<math::Tensor<float>>;
template class Embeddings<math::Tensor<double>>;
template class Embeddings<math::Tensor<fixed_point::fp32_t>>;
template class Embeddings<math::Tensor<fixed_point::fp64_t>>;
template class Embeddings<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
